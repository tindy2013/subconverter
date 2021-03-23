#ifndef INI_READER_H_INCLUDED
#define INI_READER_H_INCLUDED

#include <string>
#include <map>
#include <vector>
#include <numeric>
#include <algorithm>

#include "../codepage.h"
#include "../file_extra.h"
#include "../string.h"

enum
{
    INIREADER_EXCEPTION_EMPTY = -5,
    INIREADER_EXCEPTION_DUPLICATE,
    INIREADER_EXCEPTION_OUTOFBOUND,
    INIREADER_EXCEPTION_NOTEXIST,
    INIREADER_EXCEPTION_NOTPARSED,
    INIREADER_EXCEPTION_NONE
};

using ini_data_struct = std::map<std::string, std::multimap<std::string, std::string>>;
using string_multimap = std::multimap<std::string, std::string>;
using string_array = std::vector<std::string>;
using string_size = std::string::size_type;

class INIReader
{
    /**
    *  @brief A simple INI reader which utilize map and vector
    *  to store sections and items, allowing access in logarithmic time.
    */
private:
    /**
    *  @brief Internal parsed flag.
    */
    bool parsed = false;
    std::string current_section;
    ini_data_struct ini_content;
    string_array exclude_sections, include_sections, direct_save_sections;
    string_array section_order;

    std::string cached_section;
    ini_data_struct::iterator cached_section_content;

    std::string isolated_items_section;

    //error flags
    int last_error = INIREADER_EXCEPTION_NONE;
    unsigned int last_error_index = 0;

    inline int __priv_save_error_and_return(int x)
    {
        last_error = x;
        return last_error;
    }

    inline bool __priv_chk_ignore(const std::string &section)
    {
        bool excluded = false, included = false;
        excluded = std::find(exclude_sections.begin(), exclude_sections.end(), section) != exclude_sections.end();
        if(include_sections.size())
            included = std::find(include_sections.begin(), include_sections.end(), section) != include_sections.end();
        else
            included = true;

        return excluded || !included;
    }

    inline bool __priv_chk_direct_save(const std::string &section)
    {
        return std::find(direct_save_sections.cbegin(), direct_save_sections.cend(), section) != direct_save_sections.cend();
    }

    inline std::string __priv_get_err_str(int error)
    {
        switch(error)
        {
        case INIREADER_EXCEPTION_EMPTY:
            return "Empty document";
        case INIREADER_EXCEPTION_DUPLICATE:
            return "Duplicate section";
        case INIREADER_EXCEPTION_NOTEXIST:
            return "Target does not exist";
        case INIREADER_EXCEPTION_OUTOFBOUND:
            return "Item exists outside of any section";
        case INIREADER_EXCEPTION_NOTPARSED:
            return "Parse error";
        default:
            return "Undefined";
        }
    }

    template <typename T> inline void eraseElements(std::vector<T> &target)
    {
        target.clear();
        target.shrink_to_fit();
    }

    template <typename T> inline void eraseElements(T &target)
    {
        T().swap(target);
    }
public:
    /**
    *  @brief Set this flag to true to do a UTF8-To-GBK conversion before parsing data. Only useful in Windows.
    */
    bool do_utf8_to_gbk = false;

    /**
    *  @brief Set this flag to true so any line within the section will be stored even it doesn't follow the "name=value" format.
    * These lines will store as the name "{NONAME}".
    */
    bool store_any_line = false;

    /**
    *  @brief Save isolated items before any section definitions.
    */
    bool store_isolated_line = false;

    /**
    *  @brief Allow a section title to appear multiple times.
    */
    bool allow_dup_section_titles = false;

    /**
    *  @brief Keep an empty section while parsing.
    */
    bool keep_empty_section = true;

    /**
    *  @brief Initialize the reader.
    */
    INIReader()
    {
        parsed = false;
    }

    /**
    *  @brief Parse a file during initialization.
    */
    explicit INIReader(const std::string &filePath)
    {
        parsed = false;
        ParseFile(filePath);
    }

    ~INIReader() = default;

    INIReader& operator=(const INIReader& src)
    {
        //copy contents
        ini_content = src.ini_content;
        //copy status
        parsed = src.parsed;
        current_section = src.current_section;
        exclude_sections = src.exclude_sections;
        include_sections = src.include_sections;
        section_order = src.section_order;
        isolated_items_section = src.isolated_items_section;
        //copy preferences
        do_utf8_to_gbk = src.do_utf8_to_gbk;
        store_any_line = src.store_any_line;
        store_isolated_line = src.store_isolated_line;
        allow_dup_section_titles = src.allow_dup_section_titles;
        keep_empty_section = src.keep_empty_section;
        return *this;
    }

    INIReader(const INIReader &src) = default;

    std::string GetLastError()
    {
        if(parsed)
            return __priv_get_err_str(last_error);
        else
            return "line " + std::to_string(last_error_index) + ": " + __priv_get_err_str(last_error);
    }

    /**
    *  @brief Exclude a section with the given name.
    */
    void ExcludeSection(const std::string &section)
    {
        exclude_sections.emplace_back(section);
    }

    /**
    *  @brief Include a section with the given name.
    */
    void IncludeSection(const std::string &section)
    {
        include_sections.emplace_back(section);
    }

    /**
    *  @brief Add a section to the direct-save sections list.
    */
    void AddDirectSaveSection(const std::string &section)
    {
        direct_save_sections.emplace_back(section);
    }

    /**
    *  @brief Set isolated items to given section.
    */
    void SetIsolatedItemsSection(const std::string &section)
    {
        isolated_items_section = section;
    }

    /**
    *  @brief Parse INI content into mapped data structure.
    * If exclude sections are set, these sections will not be stored.
    * If include sections are set, only these sections will be stored.
    */
    int Parse(std::string content) //parse content into mapped data
    {
        if(!content.size()) //empty content
            return __priv_save_error_and_return(INIREADER_EXCEPTION_EMPTY);

        //remove UTF-8 BOM
        if(content.compare(0, 3, "\xEF\xBB\xBF") == 0)
            content.erase(0, 3);

        bool inExcludedSection = false, inDirectSaveSection = false, inIsolatedSection = false;
        std::string strLine, thisSection, curSection, itemName, itemVal;
        string_multimap itemGroup;
        string_array read_sections;
        std::stringstream strStrm;
        char delimiter = getLineBreak(content);

        EraseAll(); //first erase all data
        if(do_utf8_to_gbk && isStrUTF8(content))
            content = utf8ToACP(content); //do conversion if flag is set

        if(store_isolated_line && isolated_items_section.size())
        {
            curSection = isolated_items_section; //items before any section define will be store in this section
            //check this section first
            inExcludedSection = __priv_chk_ignore(curSection); //check if this section is excluded
            inDirectSaveSection = __priv_chk_direct_save(curSection); //check if this section requires direct-save
            inIsolatedSection = true;
        }
        strStrm<<content;
        last_error_index = 0; //reset error index
        while(getline(strStrm, strLine, delimiter)) //get one line of content
        {
            last_error_index++;
            strLine = trimWhitespace(strLine);
            string_size lineSize = strLine.size(), pos_equal = strLine.find("=");
            if((!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) && !inDirectSaveSection) //empty lines and comments are ignored
                continue;
            processEscapeChar(strLine);
            if(strLine[0] == '[' && strLine[lineSize - 1] == ']') //is a section title
            {
                thisSection = strLine.substr(1, lineSize - 2); //save section title
                inExcludedSection = __priv_chk_ignore(thisSection); //check if this section is excluded
                inDirectSaveSection = __priv_chk_direct_save(thisSection); //check if this section requires direct-save

                if(curSection.size() && (keep_empty_section || itemGroup.size())) //just finished reading a section
                {
                    if(ini_content.find(curSection) != ini_content.end()) //a section with the same name has been inserted
                    {
                        if(allow_dup_section_titles || !ini_content.at(curSection).size())
                        {
                            auto iter = ini_content.at(curSection); //get the existing section
                            iter.merge(itemGroup); //move new items to this section
                        }
                        else if(ini_content.at(curSection).size())
                            return __priv_save_error_and_return(INIREADER_EXCEPTION_DUPLICATE); //not allowed, stop
                    }
                    else if(!inIsolatedSection || isolated_items_section != thisSection)
                    {
                        if(itemGroup.size())
                            read_sections.push_back(curSection); //add to read sections list
                        if(std::find(section_order.cbegin(), section_order.cend(), curSection) == section_order.cend())
                            section_order.emplace_back(curSection); //add to section order if not added before
                        ini_content.emplace(std::move(curSection), std::move(itemGroup)); //insert previous section to content map
                    }
                }
                inIsolatedSection = false;
                eraseElements(itemGroup); //reset section storage
                curSection = thisSection; //start a new section
            }
            else if(((store_any_line && pos_equal == strLine.npos) || inDirectSaveSection) && !inExcludedSection && curSection.size()) //store a line without name
            {
                itemGroup.emplace("{NONAME}", strLine);
            }
            else if(pos_equal != strLine.npos) //is an item
            {
                if(inExcludedSection) //this section is excluded
                    continue;
                if(!curSection.size()) //not in any section
                    return __priv_save_error_and_return(INIREADER_EXCEPTION_OUTOFBOUND);
                string_size pos_value = strLine.find_first_not_of(' ', pos_equal + 1);
                itemName = trim(strLine.substr(0, pos_equal));
                if(pos_value != strLine.npos) //not a key with empty value
                {
                    itemVal = strLine.substr(pos_value);
                    itemGroup.emplace(std::move(itemName), std::move(itemVal)); //insert to current section
                }
                else
                    itemGroup.emplace(std::move(itemName), std::string());
            }
            if(include_sections.size() && include_sections == read_sections) //all included sections has been read
                break; //exit now
        }
        if(curSection.size() && (keep_empty_section || itemGroup.size())) //final section
        {
            if(ini_content.find(curSection) != ini_content.end()) //a section with the same name has been inserted
            {
                if(allow_dup_section_titles || isolated_items_section == thisSection)
                {
                    auto &iter = ini_content.at(curSection); //get the existing section
                    iter.merge(itemGroup); //move new items to this section
                }
                else if(ini_content.at(curSection).size())
                    return __priv_save_error_and_return(INIREADER_EXCEPTION_DUPLICATE); //not allowed, stop
            }
            else if(!inIsolatedSection || isolated_items_section != thisSection)
            {
                if(itemGroup.size())
                    read_sections.emplace_back(curSection); //add to read sections list
                if(std::find(section_order.cbegin(), section_order.cend(), curSection) == section_order.cend())
                    section_order.emplace_back(curSection); //add to section order if not added before
                ini_content.emplace(std::move(curSection), std::move(itemGroup)); //insert this section to content map
            }
        }
        parsed = true;
        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE); //all done
    }

    /**
    *  @brief Parse an INI file into mapped data structure.
    */
    int ParseFile(const std::string &filePath)
    {
        if(!fileExist(filePath))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        return Parse(fileGet(filePath));
    }

    /**
    *  @brief Check whether a section exist.
    */
    bool SectionExist(const std::string &section)
    {
        return ini_content.find(section) != ini_content.end();
    }

    /**
    *  @brief Count of sections in the whole INI.
    */
    unsigned int SectionCount()
    {
        return ini_content.size();
    }

    /**
    *  @brief Return all section names inside INI.
    */
    string_array GetSections()
    {
        return section_order;
    }

    /**
    *  @brief Enter a section with the given name. Section name and data will be cached to speed up the following reading process.
    */
    int EnterSection(const std::string &section)
    {
        if(!SectionExist(section))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        current_section = cached_section = section;
        cached_section_content = ini_content.find(section);
        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Set current section.
    */
    void SetCurrentSection(const std::string &section)
    {
        current_section = section;
    }

    /**
    *  @brief Check whether an item exist in the given section. Return false if the section does not exist.
    */
    bool ItemExist(const std::string &section, const std::string &itemName)
    {
        if(!SectionExist(section))
            return false;

        if(section != cached_section)
        {
            cached_section = section;
            cached_section_content = ini_content.find(section);
        }
        auto &cache = cached_section_content->second;
        return cache.find(itemName) != cache.end();
    }

    /**
    *  @brief Check whether an item exist in current section. Return false if the section does not exist.
    */
    bool ItemExist(const std::string &itemName)
    {
        return current_section.size() ? ItemExist(current_section, itemName) : false;
    }

    /**
    *  @brief Check whether an item with the given name prefix exist in the given section. Return false if the section does not exist.
    */
    bool ItemPrefixExist(const std::string &section, const std::string &itemName)
    {
        if(!SectionExist(section))
            return false;

        if(section != cached_section)
        {
            cached_section = section;
            cached_section_content = ini_content.find(section);
        }

        for(auto &x : cached_section_content->second)
        {
            if(x.first.find(itemName) == 0)
                return true;
        }

        return false;
    }

    /**
    *  @brief Check whether an item with the given name prefix exist in current section. Return false if the section does not exist.
    */
    bool ItemPrefixExist(const std::string &itemName)
    {
        return current_section.size() ? ItemPrefixExist(current_section, itemName) : false;
    }

    /**
    *  @brief Count of items in the given section. Return 0 if the section does not exist.
    */
    unsigned int ItemCount(const std::string &section)
    {
        if(!parsed || !SectionExist(section))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTPARSED);

        return ini_content.at(section).size();
    }

    /**
    *  @brief Erase all data from the data structure and reset parser status.
    */
    void EraseAll()
    {
        eraseElements(ini_content);
        eraseElements(section_order);
        cached_section.clear();
        cached_section_content = ini_content.end();
        parsed = false;
    }

    ini_data_struct::iterator GetItemsRef(const std::string &section)
    {
        if(!parsed || !SectionExist(section))
            return ini_content.end();

        if(cached_section != section)
        {
            cached_section = section;
            cached_section_content = ini_content.find(section);
        }
        return cached_section_content;
    }

    /**
    *  @brief Retrieve all items in the given section.
    */
    int GetItems(const std::string &section, string_multimap &data)
    {
        auto section_ref = GetItemsRef(section);
        if(section_ref == ini_content.end())
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        data = section_ref->second;
        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Retrieve all items in current section.
    */
    int GetItems(string_multimap &data)
    {
        return current_section.size() ? GetItems(current_section, data) : -1;
    }

    /**
    * @brief Retrieve item(s) with the same name prefix in the given section.
    */
    int GetAll(const std::string &section, const std::string &itemName, string_array &results) //retrieve item(s) with the same itemName prefix
    {
        if(!parsed)
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTPARSED);

        auto section_ref = GetItemsRef(section);
        if(section_ref == ini_content.end())
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        for(auto &x : section_ref->second)
        {
            if(x.first.find(itemName) == 0)
                results.emplace_back(x.second);
        }

        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    * @brief Retrieve item(s) with the same name prefix in current section.
    */
    int GetAll(const std::string &itemName, string_array &results)
    {
        return current_section.size() ? GetAll(current_section, itemName, results) : -1;
    }

    /**
    * @brief Retrieve one item with the exact same name in the given section.
    */
    std::string Get(const std::string &section, const std::string &itemName) //retrieve one item with the exact same itemName
    {
        if(!parsed || !SectionExist(section))
            return std::string();

        if(cached_section != section)
        {
            cached_section = section;
            cached_section_content = ini_content.find(section);
        }

        auto &cache = cached_section_content->second;
        auto iter = std::find_if(cache.begin(), cache.end(), [&](auto x) { return x.first == itemName; });
        if(iter != cache.end())
            return iter->second;

        return std::string();
    }

    /**
    * @brief Retrieve one item with the exact same name in current section.
    */
    std::string Get(const std::string &itemName)
    {
        return current_section.size() ? Get(current_section, itemName) : std::string();
    }

    /**
    * @brief Retrieve one item with the exact same name in the given section, if exist.
    */
    int GetIfExist(const std::string &section, const std::string &itemName, std::string &target) //retrieve one item with the exact same itemName
    {
        if(!parsed)
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTPARSED);

        if(ItemExist(section, itemName))
        {
            target = Get(section, itemName);
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
        }

        return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
    }

    /**
    * @brief Retrieve one item with the exact same name in current section, if exist.
    */
    int GetIfExist(const std::string &itemName, std::string &target)
    {
        return current_section.size() ? GetIfExist(current_section, itemName, target) : INIREADER_EXCEPTION_NOTEXIST;
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in the given section.
    */
    bool GetBool(const std::string &section, const std::string &itemName)
    {
        return Get(section, itemName) == "true";
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in current section.
    */
    bool GetBool(const std::string &itemName)
    {
        return current_section.size() ? Get(current_section, itemName) == "true" : false;
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in the given section.
    */
    int GetBoolIfExist(const std::string &section, const std::string &itemName, bool &target)
    {
        std::string result;
        int retval = GetIfExist(section, itemName, result);
        if(retval != INIREADER_EXCEPTION_NONE)
            return retval;
        if(result.size())
            target = result == "true";
        return INIREADER_EXCEPTION_NONE;
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in current section.
    */
    int GetBoolIfExist(const std::string &itemName, bool &target)
    {
        return current_section.size() ? GetBoolIfExist(current_section, itemName, target) : INIREADER_EXCEPTION_NOTEXIST;
    }

    /**
    * @brief Retrieve one number item value with the exact same name in the given section.
    */
    template <typename T> int GetNumberIfExist(const std::string &section, const std::string &itemName, T &target)
    {
        std::string result;
        int retval = GetIfExist(section, itemName, result);
        if(retval != INIREADER_EXCEPTION_NONE)
            return retval;
        if(result.size())
            target = to_number<T>(result, target);
        return INIREADER_EXCEPTION_NONE;
    }

    /**
    * @brief Retrieve one number item value with the exact same name in current section.
    */
    template <typename T> int GetNumberIfExist(const std::string &itemName, T &target)
    {
        return current_section.size() ? GetNumberIfExist(current_section, itemName, target) : INIREADER_EXCEPTION_NOTEXIST;
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in the given section.
    */
    int GetIntIfExist(const std::string &section, const std::string &itemName, int &target)
    {
        return GetNumberIfExist<int>(section, itemName, target);
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in current section.
    */
    int GetIntIfExist(const std::string &itemName, int &target)
    {
        return GetNumberIfExist<int>(itemName, target);
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in the given section.
    */
    int GetInt(const std::string &section, const std::string &itemName)
    {
        return to_int(Get(section, itemName), 0);
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in current section.
    */
    int GetInt(const std::string &itemName)
    {
        return GetInt(current_section, itemName);
    }

    /**
    * @brief Retrieve the first item found in the given section.
    */
    std::string GetFirst(const std::string &section, const std::string &itemName) //return the first item value found in section
    {
        if(!parsed)
            return std::string();
        string_array result;
        if(GetAll(section, itemName, result) != -1)
            return result[0];
        else
            return std::string();
    }

    /**
    * @brief Retrieve the first item found in current section.
    */
    std::string GetFirst(const std::string &itemName)
    {
        return current_section.size() ? GetFirst(current_section, itemName) : std::string();
    }

    /**
    * @brief Retrieve a string style array with specific separator and write into integer array.
    */
    template <typename T> void GetIntArray(const std::string &section, const std::string &itemName, const std::string &separator, T &Array)
    {
        string_array vArray;
        unsigned int index, UBound = sizeof(Array) / sizeof(Array[0]);
        vArray = split(Get(section, itemName), separator);
        for(index = 0; index < vArray.size() && index < UBound; index++)
            Array[index] = stoi(vArray[index]);
        for(; index < UBound; index++)
            Array[index] = 0;
    }

    /**
    * @brief Retrieve a string style array with specific separator and write into integer array.
    */
    template <typename T> void GetIntArray(const std::string &itemName, const std::string &separator, T &Array)
    {
        if(current_section.size())
            GetIntArray(current_section, itemName, separator, Array);
    }

    /**
    *  @brief Add a std::string value with given values.
    */
    int Set(const std::string &section, std::string itemName, std::string itemVal)
    {
        if(!section.size())
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        if(!parsed)
            parsed = true;

        if(SectionExist(section))
        {
            string_multimap &mapTemp = ini_content.at(section);
            mapTemp.insert(std::pair<std::string, std::string>(std::move(itemName), std::move(itemVal)));
        }
        else
        {
            string_multimap mapTemp;
            mapTemp.insert(std::pair<std::string, std::string>(std::move(itemName), std::move(itemVal)));
            ini_content.insert(std::pair<std::string, std::multimap<std::string, std::string>>(section, std::move(mapTemp)));
            section_order.emplace_back(section);
        }

        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Add a string value with given values.
    */
    int Set(std::string itemName, std::string itemVal)
    {
        if(!current_section.size())
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        return Set(current_section, std::move(itemName), std::move(itemVal));
    }

    /**
    *  @brief Add a boolean value with given values.
    */
    int SetBool(const std::string &section, std::string itemName, bool itemVal)
    {
        return Set(section, std::move(itemName), itemVal ? "true" : "false");
    }

    /**
    *  @brief Add a boolean value with given values.
    */
    int SetBool(std::string itemName, bool itemVal)
    {
        return SetBool(current_section, std::move(itemName), itemVal);
    }

    /**
    *  @brief Add a double value with given values.
    */
    int SetDouble(const std::string &section, std::string itemName, double itemVal)
    {
        return Set(section, std::move(itemName), std::to_string(itemVal));
    }

    /**
    *  @brief Add a double value with given values.
    */
    int SetDouble(std::string itemName, double itemVal)
    {
        return SetDouble(current_section, std::move(itemName), itemVal);
    }

    /**
    *  @brief Add a long value with given values.
    */
    int SetLong(const std::string &section, std::string itemName, long itemVal)
    {
        return Set(section, std::move(itemName), std::to_string(itemVal));
    }

    /**
    *  @brief Add a long value with given values.
    */
    int SetLong(std::string itemName, long itemVal)
    {
        return SetLong(current_section, std::move(itemName), itemVal);
    }

    /**
    *  @brief Add an array with the given separator.
    */
    template <typename T> int SetArray(const std::string &section, std::string itemName, const std::string &separator, T &Array)
    {
        std::string data;
        data = std::accumulate(std::begin(Array), std::end(Array), std::string(), [&](auto a, auto b) { return std::move(a) + std::to_string(b) + separator; });
        data.erase(data.size() - 1);
        return Set(section, std::move(itemName), data);
    }

    /**
    *  @brief Add an array with the given separator.
    */
    template <typename T> int SetArray(std::string itemName, const std::string &separator, T &Array)
    {
        return current_section.size() ? SetArray(current_section, std::move(itemName), separator, Array) : -1;
    }

    /**
    *  @brief Rename an existing section.
    */
    int RenameSection(const std::string &oldName, std::string newName)
    {
        if(!SectionExist(oldName) || SectionExist(newName))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_DUPLICATE);
        auto nodeHandler = ini_content.extract(oldName);
        nodeHandler.key() = std::move(newName);
        ini_content.insert(std::move(nodeHandler));
        std::replace(section_order.begin(), section_order.end(), oldName, newName);
        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Erase all items with the given name.
    */
    int Erase(const std::string &section, const std::string &itemName)
    {
        int retVal;
        if(!SectionExist(section))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        retVal = ini_content.at(section).erase(itemName);
        if(retVal && cached_section == section)
        {
            cached_section_content = ini_content.find(section);
        }
        return retVal;
    }

    /**
    *  @brief Erase all items with the given name.
    */
    int Erase(const std::string &itemName)
    {
        return current_section.size() ? Erase(current_section, itemName) : -1;
    }

    /**
    *  @brief Erase the first item with the given name.
    */
    int EraseFirst(const std::string &section, const std::string &itemName)
    {
        string_multimap &mapTemp = ini_content.at(section);
        string_multimap::iterator iter = mapTemp.find(itemName);
        if(iter != mapTemp.end())
        {
            mapTemp.erase(iter);
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
        }
        else
        {
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        }
    }

    /**
    *  @brief Erase the first item with the given name.
    */
    int EraseFirst(const std::string &itemName)
    {
        return current_section.size() ? EraseFirst(current_section, itemName) : -1;
    }

    /**
    *  @brief Erase all items in the given section.
    */
    void EraseSection(const std::string &section)
    {
        if(ini_content.find(section) == ini_content.end())
            return;
        eraseElements(ini_content.at(section));
        if(cached_section == section)
        {
            cached_section_content = ini_content.end();
            cached_section.erase();
        }
    }

    /**
    *  @brief Erase all items in current section.
    */
    void EraseSection()
    {
        if(current_section.size())
            EraseSection(current_section);
    }

    /**
    *  @brief Remove a section from INI.
    */
    void RemoveSection(const std::string &section)
    {
        if(ini_content.find(section) == ini_content.end())
            return;
        ini_content.erase(section);
        if(cached_section == section)
        {
            cached_section.clear();
            cached_section_content = ini_content.end();
        }
        section_order.erase(std::find(section_order.begin(), section_order.end(), section));
    }

    /**
    *  @brief Remove current section from INI.
    */
    void RemoveSection()
    {
        if(current_section.size())
            RemoveSection(current_section);
    }

    /**
    *  @brief Export the whole INI data structure into a string.
    */
    std::string ToString()
    {
        std::string content, itemVal;

        if(!parsed)
            return std::string();

        for(auto &x : section_order)
        {
            string_size strsize = 0;
            content += "[" + x + "]\n";
            if(ini_content.find(x) != ini_content.end())
            {
                auto section = ini_content.at(x);
                if(section.empty())
                {
                    content += "\n";
                    continue;
                }
                for(auto iter = section.begin(); iter != section.end(); iter++)
                {
                    if(iter->first != "{NONAME}")
                        content += iter->first + "=";
                    itemVal = iter->second;
                    processEscapeCharReverse(itemVal);
                    content += itemVal + "\n";
                    if(std::next(iter) == section.end())
                        strsize = itemVal.size();
                }
            }
            if(strsize)
                content += "\n";
        }
        return content;
    }

    /**
    *  @brief Export the whole INI data structure into a file.
    */
    int ToFile(const std::string &filePath)
    {
        return fileWrite(filePath, ToString(), true);
    }
};

#endif // INI_READER_H_INCLUDED

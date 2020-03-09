#ifndef INI_READER_H_INCLUDED
#define INI_READER_H_INCLUDED

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>

#include "misc.h"

enum
{
    INIREADER_EXCEPTION_EMPTY = -5,
    INIREADER_EXCEPTION_DUPLICATE,
    INIREADER_EXCEPTION_OUTOFBOUND,
    INIREADER_EXCEPTION_NOTEXIST,
    INIREADER_EXCEPTION_NOTPARSED,
    INIREADER_EXCEPTION_NONE
};

typedef std::map<std::string, std::multimap<std::string, std::string>> ini_data_struct;
typedef std::multimap<std::string, std::string> string_multimap;
typedef std::vector<std::string> string_array;

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
    string_array read_sections, section_order;

    std::string cached_section;
    string_multimap cached_section_content;

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
        excluded = std::find(exclude_sections.cbegin(), exclude_sections.cend(), section) != exclude_sections.cend();
        if(include_sections.size())
            included = std::find(include_sections.cbegin(), include_sections.cend(), section) != include_sections.cend();
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
    *  @brief Keep an empty section while parsing
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
    INIReader(std::string filePath)
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
        read_sections = src.read_sections;
        section_order = src.section_order;
        isolated_items_section = src.isolated_items_section;
        //copy preferences
        do_utf8_to_gbk = src.do_utf8_to_gbk;
        store_any_line = src.store_any_line;
        store_isolated_line = src.store_isolated_line;
        allow_dup_section_titles = src.allow_dup_section_titles;
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
    void ExcludeSection(std::string section)
    {
        exclude_sections.emplace_back(section);
    }

    /**
    *  @brief Include a section with the given name.
    */
    void IncludeSection(std::string section)
    {
        include_sections.emplace_back(section);
    }

    /**
    *  @brief Add a section to the direct-save sections list.
    */
    void AddDirectSaveSection(std::string section)
    {
        direct_save_sections.emplace_back(section);
    }

    /**
    *  @brief Set isolated items to given section.
    */
    void SetIsolatedItemsSection(std::string section)
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

        bool inExcludedSection = false, inDirectSaveSection = false;
        std::string strLine, thisSection, curSection, itemName, itemVal;
        string_multimap itemGroup, existItemGroup;
        std::stringstream strStrm;
        unsigned int lineSize = 0;
        char delimiter = count(content.begin(), content.end(), '\n') < 1 ? '\r' : '\n';

        EraseAll(); //first erase all data
        if(do_utf8_to_gbk && is_str_utf8(content))
            content = UTF8ToACP(content); //do conversion if flag is set

        if(store_isolated_line)
        {
            curSection = isolated_items_section; //items before any section define will be store in this section
            //check this section first
            inExcludedSection = __priv_chk_ignore(curSection); //check if this section is excluded
            inDirectSaveSection = __priv_chk_direct_save(curSection); //check if this section requires direct-save
        }
        strStrm<<content;
        last_error_index = 0; //reset error index
        while(getline(strStrm, strLine, delimiter)) //get one line of content
        {
            last_error_index++;
            lineSize = strLine.size();
            if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
            {
                strLine.erase(lineSize - 1);
                lineSize--;
            }
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            if(strLine[0] == '[' && strLine[lineSize - 1] == ']') //is a section title
            {
                thisSection = strLine.substr(1, lineSize - 2); //save section title
                inExcludedSection = __priv_chk_ignore(thisSection); //check if this section is excluded
                inDirectSaveSection = __priv_chk_direct_save(thisSection); //check if this section requires direct-save

                if(curSection.size() && (keep_empty_section || itemGroup.size())) //just finished reading a section
                {
                    if(ini_content.count(curSection)) //a section with the same name has been inserted
                    {
                        if(allow_dup_section_titles || !ini_content.at(curSection).size())
                        {
                            eraseElements(existItemGroup);
                            existItemGroup = ini_content.at(curSection); //get the old items
                            for(auto &x : existItemGroup)
                                itemGroup.emplace(x); //insert them all into new section
                        }
                        else if(ini_content.at(curSection).size())
                            return __priv_save_error_and_return(INIREADER_EXCEPTION_DUPLICATE); //not allowed, stop
                        ini_content.erase(curSection); //remove the old section
                    }
                    else if(itemGroup.size())
                        read_sections.push_back(curSection); //add to read sections list
                    ini_content.emplace(curSection, itemGroup); //insert previous section to content map
                    if(std::count(section_order.cbegin(), section_order.cend(), curSection) == 0)
                        section_order.emplace_back(curSection);
                }

                eraseElements(itemGroup); //reset section storage
                curSection = thisSection; //start a new section
            }
            else if(((store_any_line && strLine.find("=") == strLine.npos) || inDirectSaveSection) && !inExcludedSection && curSection.size()) //store a line without name
            {
                itemGroup.emplace("{NONAME}", strLine);
            }
            else if(strLine.find("=") != strLine.npos) //is an item
            {
                if(inExcludedSection) //this section is excluded
                    continue;
                if(!curSection.size()) //not in any section
                    return __priv_save_error_and_return(INIREADER_EXCEPTION_OUTOFBOUND);
                itemName = trim(strLine.substr(0, strLine.find("=")));
                itemVal = trim(strLine.substr(strLine.find("=") + 1));
                itemGroup.emplace(itemName, itemVal); //insert to current section
            }
            if(include_sections.size() && include_sections == read_sections) //all included sections has been read
                break; //exit now
        }
        if(curSection.size() && (keep_empty_section || itemGroup.size())) //final section
        {
            if(ini_content.count(curSection)) //a section with the same name has been inserted
            {
                if(allow_dup_section_titles)
                {
                    eraseElements(existItemGroup);
                    existItemGroup = ini_content.at(curSection); //get the old items
                    for(auto &x : existItemGroup)
                        itemGroup.emplace(x); //insert them all into new section
                }
                else if(ini_content.at(curSection).size())
                    return __priv_save_error_and_return(INIREADER_EXCEPTION_DUPLICATE); //not allowed, stop
                ini_content.erase(curSection); //remove the old section
            }
            else if(itemGroup.size())
                read_sections.emplace_back(curSection); //add to read sections list
            ini_content.emplace(curSection, itemGroup); //insert this section to content map
            if(std::count(section_order.cbegin(), section_order.cend(), curSection) == 0)
                section_order.emplace_back(curSection);
        }
        parsed = true;
        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE); //all done
    }

    /**
    *  @brief Parse an INI file into mapped data structure.
    */
    int ParseFile(std::string filePath)
    {
        if(!fileExist(filePath))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        return Parse(fileGet(filePath));
    }

    /**
    *  @brief Check whether a section exist.
    */
    bool SectionExist(std::string section)
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
        string_array retData;

        for(auto &x : ini_content)
        {
            retData.emplace_back(x.first);
        }
        //std::transform(ini_content.begin(), ini_content.end(), back_inserter(retData), [](auto x) -> std::string {return x.first;});

        return retData;
    }

    /**
    *  @brief Enter a section with the given name. Section name and data will be cached to speed up the following reading process.
    */
    int EnterSection(std::string section)
    {
        if(!SectionExist(section))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        current_section = cached_section = section;
        cached_section_content = ini_content.at(section);
        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Set current section.
    */
    void SetCurrentSection(std::string section)
    {
        current_section = section;
    }

    /**
    *  @brief Check whether an item exist in the given section. Return false if the section does not exist.
    */
    bool ItemExist(std::string section, std::string itemName)
    {
        if(!SectionExist(section))
            return false;

        if(section != cached_section)
        {
            cached_section = section;
            cached_section_content= ini_content.at(section);
        }

        return cached_section_content.find(itemName) != cached_section_content.end();
    }

    /**
    *  @brief Check whether an item exist in current section. Return false if the section does not exist.
    */
    bool ItemExist(std::string itemName)
    {
        return current_section.size() ? ItemExist(current_section, itemName) : false;
    }

    /**
    *  @brief Check whether an item with the given name prefix exist in the given section. Return false if the section does not exist.
    */
    bool ItemPrefixExist(std::string section, std::string itemName)
    {
        if(!SectionExist(section))
            return false;

        if(section != cached_section)
        {
            cached_section = section;
            cached_section_content= ini_content.at(section);
        }

        for(auto &x : cached_section_content)
        {
            if(x.first.find(itemName) == 0)
                return true;
        }

        return false;
    }

    /**
    *  @brief Check whether an item with the given name prefix exist in current section. Return false if the section does not exist.
    */
    bool ItemPrefixExist(std::string itemName)
    {
        return current_section.size() ? ItemPrefixExist(current_section, itemName) : false;
    }

    /**
    *  @brief Count of items in the given section. Return 0 if the section does not exist.
    */
    unsigned int ItemCount(std::string section)
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
        parsed = false;
    }

    /**
    *  @brief Retrieve all items in the given section.
    */
    int GetItems(std::string section, string_multimap &data)
    {
        if(!parsed || !SectionExist(section))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        if(cached_section != section)
        {
            cached_section = section;
            cached_section_content = ini_content.at(section);
        }

        data = cached_section_content;
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
    int GetAll(std::string section, std::string itemName, string_array &results) //retrieve item(s) with the same itemName prefix
    {
        if(!parsed)
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTPARSED);

        string_multimap mapTemp;

        if(GetItems(section, mapTemp) != 0)
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        for(auto &x : mapTemp)
        {
            if(x.first.find(itemName) == 0)
                results.emplace_back(x.second);
        }

        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    * @brief Retrieve item(s) with the same name prefix in current section.
    */
    int GetAll(std::string itemName, string_array &results)
    {
        return current_section.size() ? GetAll(current_section, itemName, results) : -1;
    }

    /**
    * @brief Retrieve one item with the exact same name in the given section.
    */
    std::string Get(std::string section, std::string itemName) //retrieve one item with the exact same itemName
    {
        if(!parsed)
            return std::string();

        string_multimap mapTemp;

        if(GetItems(section, mapTemp) != 0)
            return std::string();

        for(auto &x : mapTemp)
        {
            if(x.first == itemName)
                return x.second;
        }

        return std::string();
    }

    /**
    * @brief Retrieve one item with the exact same name in current section.
    */
    std::string Get(std::string itemName)
    {
        return current_section.size() ? Get(current_section, itemName) : std::string();
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in the given section.
    */
    bool GetBool(std::string section, std::string itemName)
    {
        return Get(section, itemName) == "true";
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in current section.
    */
    bool GetBool(std::string itemName)
    {
        return current_section.size() ? Get(current_section, itemName) == "true" : false;
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in the given section.
    */
    int GetInt(std::string section, std::string itemName)
    {
        return stoi(Get(section, itemName));
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in current section.
    */
    int GetInt(std::string itemName)
    {
        return GetInt(current_section, itemName);
    }

    /**
    * @brief Retrieve the first item found in the given section.
    */
    std::string GetFirst(std::string section, std::string itemName) //return the first item value found in section
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
    std::string GetFirst(std::string itemName)
    {
        return current_section.size() ? GetFirst(current_section, itemName) : std::string();
    }

    /**
    * @brief Retrieve a string style array with specific separator and write into integer array.
    */
    template <typename T> void GetIntArray(std::string section, std::string itemName, std::string separator, T &Array)
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
    template <typename T> void GetIntArray(std::string itemName, std::string separator, T &Array)
    {
        if(current_section.size())
            GetIntArray(current_section, itemName, separator, Array);
    }

    /**
    *  @brief Add a std::string value with given values.
    */
    int Set(std::string section, std::string itemName, std::string itemVal)
    {
        if(!section.size())
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        if(!parsed)
            parsed = true;

        if(SectionExist(section))
        {
            string_multimap &mapTemp = ini_content.at(section);
            mapTemp.insert(std::pair<std::string, std::string>(itemName, itemVal));
        }
        else
        {
            string_multimap mapTemp;
            mapTemp.insert(std::pair<std::string, std::string>(itemName, itemVal));
            ini_content.insert(std::pair<std::string, std::multimap<std::string, std::string>>(section, mapTemp));
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
        return Set(current_section, itemName, itemVal);
    }

    /**
    *  @brief Add a boolean value with given values.
    */
    int SetBool(std::string section, std::string itemName, bool itemVal)
    {
        return Set(section, itemName, itemVal ? "true" : "false");
    }

    /**
    *  @brief Add a boolean value with given values.
    */
    int SetBool(std::string itemName, bool itemVal)
    {
        return SetBool(current_section, itemName, itemVal);
    }

    /**
    *  @brief Add a double value with given values.
    */
    int SetDouble(std::string section, std::string itemName, double itemVal)
    {
        return Set(section, itemName, std::to_string(itemVal));
    }

    /**
    *  @brief Add a double value with given values.
    */
    int SetDouble(std::string itemName, double itemVal)
    {
        return SetDouble(current_section, itemName, itemVal);
    }

    /**
    *  @brief Add a long value with given values.
    */
    int SetLong(std::string section, std::string itemName, long itemVal)
    {
        return Set(section, itemName, std::to_string(itemVal));
    }

    /**
    *  @brief Add a long value with given values.
    */
    int SetLong(std::string itemName, long itemVal)
    {
        return SetLong(current_section, itemName, itemVal);
    }

    /**
    *  @brief Add an array with the given separator.
    */
    template <typename T> int SetArray(std::string section, std::string itemName, std::string separator, T &Array)
    {
        std::string data;
        for(auto &x : Array)
            data += std::to_string(x) + separator;
        data = data.substr(0, data.size() - separator.size());
        return Set(section, itemName, data);
    }

    /**
    *  @brief Add an array with the given separator.
    */
    template <typename T> int SetArray(std::string itemName, std::string separator, T &Array)
    {
        return current_section.size() ? SetArray(current_section, itemName, separator, Array) : -1;
    }

    int RenameSection(std::string oldName, std::string newName)
    {
        if(!SectionExist(oldName) || SectionExist(newName))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_DUPLICATE);
        /*
        auto nodeHandler = ini_content.extract(oldName);
        nodeHandler.key() = newName;
        ini_content.insert(std::move(nodeHandler));
        */
        ini_content[newName] = std::move(ini_content[oldName]);
        ini_content.erase(oldName);
        std::replace(section_order.begin(), section_order.end(), oldName, newName);
        return __priv_save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Erase all items with the given name.
    */
    int Erase(std::string section, std::string itemName)
    {
        int retVal;
        if(!SectionExist(section))
            return __priv_save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        retVal = ini_content.at(section).erase(itemName);
        if(retVal && cached_section == section)
        {
            cached_section_content = ini_content.at(section);
        }
        return retVal;
    }

    /**
    *  @brief Erase all items with the given name.
    */
    int Erase(std::string itemName)
    {
        return current_section.size() ? Erase(current_section, itemName) : -1;
    }

    /**
    *  @brief Erase the first item with the given name.
    */
    int EraseFirst(std::string section, std::string itemName)
    {
        string_multimap &mapTemp = ini_content.at(section);
        string_multimap::iterator iter = mapTemp.find(itemName);
        if(iter != mapTemp.end())
        {
            mapTemp.erase(iter);
            if(cached_section == section)
            {
                cached_section_content = mapTemp;
            }
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
    int EraseFirst(std::string itemName)
    {
        return current_section.size() ? EraseFirst(current_section, itemName) : -1;
    }

    /**
    *  @brief Erase all items in the given section.
    */
    void EraseSection(std::string section)
    {
        if(ini_content.find(section) == ini_content.end())
            return;
        eraseElements(ini_content.at(section));
        if(cached_section == section)
        {
            eraseElements(cached_section_content);
            cached_section.erase();
        }
        //section_order.erase(std::find(section_order.begin(), section_order.end(), section));
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
    *  @brief Export the whole INI data structure into a string.
    */
    std::string ToString()
    {
        std::string content;

        if(!parsed)
            return std::string();

        for(auto &x : section_order)
        {
            content += "[" + x + "]\n";
            if(ini_content.find(x) != ini_content.end())
            {
                for(auto &y : ini_content.at(x))
                {
                    if(y.first != "{NONAME}")
                        content += y.first + "=";
                    content += y.second + "\n";
                }
            }
            content += "\n";
        }

        return content.erase(content.size() - 2);
    }

    /**
    *  @brief Export the whole INI data structure into a file.
    */
    int ToFile(std::string filePath)
    {
        return fileWrite(filePath, ToString(), true);
    }
};

#endif // INI_READER_H_INCLUDED

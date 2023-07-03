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

    inline int save_error_and_return(int x)
    {
        last_error = x;
        return last_error;
    }

    inline bool chk_ignore(const std::string &section)
    {
        bool excluded = false, included = false;
        excluded = std::find(exclude_sections.begin(), exclude_sections.end(), section) != exclude_sections.end();
        if(!include_sections.empty())
            included = std::find(include_sections.begin(), include_sections.end(), section) != include_sections.end();
        else
            included = true;

        return excluded || !included;
    }

    inline bool chk_direct_save(const std::string &section)
    {
        return std::find(direct_save_sections.cbegin(), direct_save_sections.cend(), section) != direct_save_sections.cend();
    }

    inline std::string get_err_str(int error)
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
            return "parse error";
        default:
            return "Undefined";
        }
    }

    template <typename T> inline void erase_elements(std::vector<T> &target)
    {
        target.clear();
        target.shrink_to_fit();
    }

    template <typename T> inline void erase_elements(T &target)
    {
        T().swap(target);
    }
public:
    /**
    *  @brief set this flag to true to do a UTF8-To-GBK conversion before parsing data. Only useful in Windows.
    */
    bool do_utf8_to_gbk = false;

    /**
    *  @brief set this flag to true so any line within the section will be stored even it doesn't follow the "name=value" format.
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
    *  @brief parse a file during initialization.
    */
    explicit INIReader(const std::string &filePath)
    {
        parsed = false;
        parse_file(filePath);
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

    std::string get_last_error()
    {
        if(parsed)
            return get_err_str(last_error);
        else
            return "line " + std::to_string(last_error_index) + ": " + get_err_str(last_error);
    }

    /**
    *  @brief Exclude a section with the given name.
    */
    void exclude_section(const std::string &section)
    {
        exclude_sections.emplace_back(section);
    }

    /**
    *  @brief Include a section with the given name.
    */
    void include_section(const std::string &section)
    {
        include_sections.emplace_back(section);
    }

    /**
    *  @brief Add a section to the direct-save sections list.
    */
    void add_direct_save_section(const std::string &section)
    {
        direct_save_sections.emplace_back(section);
    }

    /**
    *  @brief set isolated items to given section.
    */
    void set_isolated_items_section(const std::string &section)
    {
        isolated_items_section = section;
    }

    /**
    *  @brief parse INI content into mapped data structure.
    * If exclude sections are set, these sections will not be stored.
    * If include sections are set, only these sections will be stored.
    */
    int parse(std::string content) //parse content into mapped data
    {
        if(content.empty()) //empty content
            return save_error_and_return(INIREADER_EXCEPTION_EMPTY);

        //remove UTF-8 BOM
        if(content.compare(0, 3, "\xEF\xBB\xBF") == 0)
            content.erase(0, 3);

        bool inExcludedSection = false, inDirectSaveSection = false, inIsolatedSection = false;
        std::string strLine, thisSection, curSection, itemName, itemVal;
        string_multimap itemGroup;
        string_array read_sections;
        std::stringstream strStrm;
        char delimiter = getLineBreak(content);

        erase_all(); //first erase all data
        if(do_utf8_to_gbk && isStrUTF8(content))
            content = utf8ToACP(content); //do conversion if flag is set

        if(store_isolated_line && !isolated_items_section.empty())
        {
            curSection = isolated_items_section; //items before any section define will be store in this section
            //check this section first
            inExcludedSection = chk_ignore(curSection); //check if this section is excluded
            inDirectSaveSection = chk_direct_save(curSection); //check if this section requires direct-save
            inIsolatedSection = true;
        }
        strStrm<<content;
        last_error_index = 0; //reset error index
        while(getline(strStrm, strLine, delimiter)) //get one line of content
        {
            last_error_index++;
            strLine = trimWhitespace(strLine);
            string_size lineSize = strLine.size(), pos_equal = strLine.find('=');
            if((!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) && !inDirectSaveSection) //empty lines and comments are ignored
                continue;
            processEscapeChar(strLine);
            if(strLine[0] == '[' && strLine[lineSize - 1] == ']') //is a section title
            {
                thisSection = strLine.substr(1, lineSize - 2); //save section title
                inExcludedSection = chk_ignore(thisSection); //check if this section is excluded
                inDirectSaveSection = chk_direct_save(thisSection); //check if this section requires direct-save

                if(!curSection.empty() && (keep_empty_section || !itemGroup.empty())) //just finished reading a section
                {
                    if(ini_content.find(curSection) != ini_content.end()) //a section with the same name has been inserted
                    {
                        if(allow_dup_section_titles || ini_content.at(curSection).empty())
                        {
                            auto iter = ini_content.at(curSection); //get the existing section
                            iter.merge(itemGroup); //move new items to this section
                        }
                        else if(!ini_content.at(curSection).empty())
                            return save_error_and_return(INIREADER_EXCEPTION_DUPLICATE); //not allowed, stop
                    }
                    else if(!inIsolatedSection || isolated_items_section != thisSection)
                    {
                        if(!itemGroup.empty())
                            read_sections.push_back(curSection); //add to read sections list
                        if(std::find(section_order.cbegin(), section_order.cend(), curSection) == section_order.cend())
                            section_order.emplace_back(curSection); //add to section order if not added before
                        ini_content.emplace(std::move(curSection), std::move(itemGroup)); //insert previous section to content map
                    }
                }
                inIsolatedSection = false;
                erase_elements(itemGroup); //reset section storage
                curSection = thisSection; //start a new section
            }
            else if(((store_any_line && pos_equal == std::string::npos) || inDirectSaveSection) && !inExcludedSection && !curSection.empty()) //store a line without name
            {
                itemGroup.emplace("{NONAME}", strLine);
            }
            else if(pos_equal != std::string::npos) //is an item
            {
                if(inExcludedSection) //this section is excluded
                    continue;
                if(curSection.empty()) //not in any section
                    return save_error_and_return(INIREADER_EXCEPTION_OUTOFBOUND);
                string_size pos_value = strLine.find_first_not_of(' ', pos_equal + 1);
                itemName = trim(strLine.substr(0, pos_equal));
                if(pos_value != std::string::npos) //not a key with empty value
                {
                    itemVal = strLine.substr(pos_value);
                    itemGroup.emplace(std::move(itemName), std::move(itemVal)); //insert to current section
                }
                else
                    itemGroup.emplace(std::move(itemName), "");
            }
            if(!include_sections.empty() && include_sections == read_sections) //all included sections has been read
                break; //exit now
        }
        if(!curSection.empty() && (keep_empty_section || !itemGroup.empty())) //final section
        {
            if(ini_content.find(curSection) != ini_content.end()) //a section with the same name has been inserted
            {
                if(allow_dup_section_titles || isolated_items_section == thisSection)
                {
                    auto &iter = ini_content.at(curSection); //get the existing section
                    iter.merge(itemGroup); //move new items to this section
                }
                else if(!ini_content.at(curSection).empty())
                    return save_error_and_return(INIREADER_EXCEPTION_DUPLICATE); //not allowed, stop
            }
            else if(!inIsolatedSection || isolated_items_section != thisSection)
            {
                if(!itemGroup.empty())
                    read_sections.emplace_back(curSection); //add to read sections list
                if(std::find(section_order.cbegin(), section_order.cend(), curSection) == section_order.cend())
                    section_order.emplace_back(curSection); //add to section order if not added before
                ini_content.emplace(std::move(curSection), std::move(itemGroup)); //insert this section to content map
            }
        }
        parsed = true;
        return save_error_and_return(INIREADER_EXCEPTION_NONE); //all done
    }

    /**
    *  @brief parse an INI file into mapped data structure.
    */
    int parse_file(const std::string &filePath)
    {
        if(!fileExist(filePath))
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        return parse(fileGet(filePath));
    }

    /**
    *  @brief Check whether a section exist.
    */
    bool section_exist(const std::string &section)
    {
        return ini_content.find(section) != ini_content.end();
    }

    /**
    *  @brief Count of sections in the whole INI.
    */
    unsigned int section_count()
    {
        return ini_content.size();
    }

    /**
    *  @brief Return all section names inside INI.
    */
    string_array get_section_names()
    {
        return section_order;
    }

    /**
    *  @brief Enter a section with the given name. Section name and data will be cached to speed up the following reading process.
    */
    int enter_section(const std::string &section)
    {
        if(!section_exist(section))
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        current_section = cached_section = section;
        cached_section_content = ini_content.find(section);
        return save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief set current section.
    */
    void set_current_section(const std::string &section)
    {
        current_section = section;
    }

    /**
    *  @brief Check whether an item exist in the given section. Return false if the section does not exist.
    */
    bool item_exist(const std::string &section, const std::string &itemName)
    {
        if(!section_exist(section))
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
    bool item_exist(const std::string &itemName)
    {
        return !current_section.empty() && item_exist(current_section, itemName);
    }

    /**
    *  @brief Check whether an item with the given name prefix exist in the given section. Return false if the section does not exist.
    */
    bool item_prefix_exists(const std::string &section, const std::string &itemName)
    {
        if(!section_exist(section))
            return false;

        if(section != cached_section)
        {
            cached_section = section;
            cached_section_content = ini_content.find(section);
        }

        auto &items = cached_section_content->second;

        return std::any_of(items.cbegin(), items.cend(), [&](auto &x) {
            return x.first.find(itemName) == 0;
        });
    }

    /**
    *  @brief Check whether an item with the given name prefix exist in current section. Return false if the section does not exist.
    */
    bool item_prefix_exist(const std::string &itemName)
    {
        return !current_section.empty() && item_prefix_exists(current_section, itemName);
    }

    /**
    *  @brief Count of items in the given section. Return 0 if the section does not exist.
    */
    unsigned int item_count(const std::string &section)
    {
        if(!parsed || !section_exist(section))
            return save_error_and_return(INIREADER_EXCEPTION_NOTPARSED);

        return ini_content.at(section).size();
    }

    /**
    *  @brief erase all data from the data structure and reset parser status.
    */
    void erase_all()
    {
        erase_elements(ini_content);
        erase_elements(section_order);
        cached_section.clear();
        cached_section_content = ini_content.end();
        parsed = false;
    }

    ini_data_struct::iterator get_items_ref(const std::string &section)
    {
        if(!parsed || !section_exist(section))
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
    int get_items(const std::string &section, string_multimap &data)
    {
        auto section_ref = get_items_ref(section);
        if(section_ref == ini_content.end())
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        data = section_ref->second;
        return save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Retrieve all items in current section.
    */
    int get_items(string_multimap &data)
    {
        return !current_section.empty() ? get_items(current_section, data) : -1;
    }

    /**
    * @brief Retrieve item(s) with the same name prefix in the given section.
    */
    int get_all(const std::string &section, const std::string &itemName, string_array &results) //retrieve item(s) with the same itemName prefix
    {
        if(!parsed)
            return save_error_and_return(INIREADER_EXCEPTION_NOTPARSED);

        auto section_ref = get_items_ref(section);
        if(section_ref == ini_content.end())
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        for(auto &x : section_ref->second)
        {
            if(x.first.find(itemName) == 0)
                results.emplace_back(x.second);
        }

        return save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    * @brief Retrieve item(s) with the same name prefix in current section.
    */
    int get_all(const std::string &itemName, string_array &results)
    {
        return !current_section.empty() ? get_all(current_section, itemName, results) : -1;
    }

    /**
    * @brief Retrieve one item with the exact same name in the given section.
    */
    std::string get(const std::string &section, const std::string &itemName) //retrieve one item with the exact same itemName
    {
        if(!parsed || !section_exist(section))
            return "";

        if(cached_section != section)
        {
            cached_section = section;
            cached_section_content = ini_content.find(section);
        }

        auto &cache = cached_section_content->second;
        auto iter = std::find_if(cache.begin(), cache.end(), [&](auto x) { return x.first == itemName; });
        if(iter != cache.end())
            return iter->second;

        return "";
    }

    /**
    * @brief Retrieve one item with the exact same name in current section.
    */
    std::string get(const std::string &itemName)
    {
        return !current_section.empty() ? get(current_section, itemName) : "";
    }

    /**
    * @brief Retrieve one item with the exact same name in the given section, if exist.
    */
    int get_if_exist(const std::string &section, const std::string &itemName, std::string &target) //retrieve one item with the exact same itemName
    {
        if(!parsed)
            return save_error_and_return(INIREADER_EXCEPTION_NOTPARSED);

        if(item_exist(section, itemName))
        {
            target = get(section, itemName);
            return save_error_and_return(INIREADER_EXCEPTION_NONE);
        }

        return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
    }

    /**
    * @brief Retrieve one item with the exact same name in current section, if exist.
    */
    int get_if_exist(const std::string &itemName, std::string &target)
    {
        return !current_section.empty() ? get_if_exist(current_section, itemName, target) : INIREADER_EXCEPTION_NOTEXIST;
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in the given section.
    */
    bool get_bool(const std::string &section, const std::string &itemName)
    {
        return get(section, itemName) == "true";
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in current section.
    */
    bool get_bool(const std::string &itemName)
    {
        return !current_section.empty() && get(current_section, itemName) == "true";
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in the given section.
    */
    int get_bool_if_exist(const std::string &section, const std::string &itemName, bool &target)
    {
        std::string result;
        int retVal = get_if_exist(section, itemName, result);
        if(retVal != INIREADER_EXCEPTION_NONE)
            return retVal;
        if(!result.empty())
            target = result == "true";
        return INIREADER_EXCEPTION_NONE;
    }

    /**
    * @brief Retrieve one boolean item value with the exact same name in current section.
    */
    int get_bool_if_exist(const std::string &itemName, bool &target)
    {
        return !current_section.empty() ? get_bool_if_exist(current_section, itemName, target) : INIREADER_EXCEPTION_NOTEXIST;
    }

    /**
    * @brief Retrieve one number item value with the exact same name in the given section.
    */
    template <typename T> int get_number_if_exist(const std::string &section, const std::string &itemName, T &target)
    {
        std::string result;
        int retVal = get_if_exist(section, itemName, result);
        if(retVal != INIREADER_EXCEPTION_NONE)
            return retVal;
        if(!result.empty())
            target = to_number<T>(result, target);
        return INIREADER_EXCEPTION_NONE;
    }

    /**
    * @brief Retrieve one number item value with the exact same name in current section.
    */
    template <typename T> int get_number_if_exist(const std::string &itemName, T &target)
    {
        return !current_section.empty() ? get_number_if_exist(current_section, itemName, target) : INIREADER_EXCEPTION_NOTEXIST;
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in the given section.
    */
    int get_int_if_exist(const std::string &section, const std::string &itemName, int &target)
    {
        return get_number_if_exist<int>(section, itemName, target);
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in current section.
    */
    int get_int_if_exist(const std::string &itemName, int &target)
    {
        return get_number_if_exist<int>(itemName, target);
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in the given section.
    */
    int get_int(const std::string &section, const std::string &itemName)
    {
        return to_int(get(section, itemName), 0);
    }

    /**
    * @brief Retrieve one integer item value with the exact same name in current section.
    */
    int get_int(const std::string &itemName)
    {
        return get_int(current_section, itemName);
    }

    /**
    * @brief Retrieve the first item found in the given section.
    */
    std::string get_first(const std::string &section, const std::string &itemName) //return the first item value found in section
    {
        if(!parsed)
            return "";
        string_array result;
        if(get_all(section, itemName, result) != -1)
            return result[0];
        else
            return "";
    }

    /**
    * @brief Retrieve the first item found in current section.
    */
    std::string get_first(const std::string &itemName)
    {
        return !current_section.empty() ? get_first(current_section, itemName) : "";
    }

    /**
    * @brief Retrieve a string style array with specific separator and write into integer array.
    */
    template <typename T, size_t N> void get_int_array(const std::string &section, const std::string &itemName, const std::string &separator, T& Array)
    {
        unsigned int index, UBound = sizeof(Array[0]) / sizeof(Array);
        string_array vArray = split(get(section, itemName), separator);
        for(index = 0; index < vArray.size() && index < UBound; index++)
            Array[index] = stoi(vArray[index]);
        for(; index < UBound; index++)
            Array[index] = 0;
    }

    /**
    * @brief Retrieve a string style array with specific separator and write into integer array.
    */
    template <typename T> void get_int_array(const std::string &itemName, const std::string &separator, T& Array)
    {
        if(!current_section.empty())
            get_int_array(current_section, itemName, separator, Array);
    }

    /**
    *  @brief Add a std::string value with given values.
    */
    int set(const std::string &section, std::string itemName, std::string itemVal)
    {
        if(section.empty())
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        if(!parsed)
            parsed = true;

        if(section_exist(section))
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

        return save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief Add a string value with given values.
    */
    int set(std::string itemName, std::string itemVal)
    {
        if(current_section.empty())
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        return set(current_section, std::move(itemName), std::move(itemVal));
    }

    /**
    *  @brief Add a boolean value with given values.
    */
    int set_bool(const std::string &section, std::string itemName, bool itemVal)
    {
        return set(section, std::move(itemName), itemVal ? "true" : "false");
    }

    /**
    *  @brief Add a boolean value with given values.
    */
    int set_bool(std::string itemName, bool itemVal)
    {
        return set_bool(current_section, std::move(itemName), itemVal);
    }

    /**
    *  @brief Add a double value with given values.
    */
    int set_double(const std::string &section, std::string itemName, double itemVal)
    {
        return set(section, std::move(itemName), std::to_string(itemVal));
    }

    /**
    *  @brief Add a double value with given values.
    */
    int set_double(std::string itemName, double itemVal)
    {
        return set_double(current_section, std::move(itemName), itemVal);
    }

    /**
    *  @brief Add a long value with given values.
    */
    int set_long(const std::string &section, std::string itemName, long itemVal)
    {
        return set(section, std::move(itemName), std::to_string(itemVal));
    }

    /**
    *  @brief Add a long value with given values.
    */
    int set_long(std::string itemName, long itemVal)
    {
        return set_long(current_section, std::move(itemName), itemVal);
    }

    /**
    *  @brief Add an array with the given separator.
    */
    template <typename T> int set_array(const std::string &section, std::string itemName, const std::string &separator, T &Array)
    {
        std::string data;
        data = std::accumulate(std::begin(Array), std::end(Array), std::string(), [&](auto a, auto b) { return std::move(a) + std::to_string(b) + separator; });
        data.erase(data.size() - 1);
        return set(section, std::move(itemName), data);
    }

    /**
    *  @brief Add an array with the given separator.
    */
    template <typename T> int set_array(std::string itemName, const std::string &separator, T &Array)
    {
        return !current_section.empty() ? set_array(current_section, std::move(itemName), separator, Array) : -1;
    }

    /**
    *  @brief Rename an existing section.
    */
    int rename_section(const std::string &oldName, const std::string& newName)
    {
        if(!section_exist(oldName) || section_exist(newName))
            return save_error_and_return(INIREADER_EXCEPTION_DUPLICATE);
        auto nodeHandler = ini_content.extract(oldName);
        nodeHandler.key() = newName;
        ini_content.insert(std::move(nodeHandler));
        std::replace(section_order.begin(), section_order.end(), oldName, newName);
        return save_error_and_return(INIREADER_EXCEPTION_NONE);
    }

    /**
    *  @brief erase all items with the given name.
    */
    int erase(const std::string &section, const std::string &itemName)
    {
        if(!section_exist(section))
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);

        auto retVal = ini_content.at(section).erase(itemName);
        if(retVal && cached_section == section)
        {
            cached_section_content = ini_content.find(section);
        }
        return retVal;
    }

    /**
    *  @brief erase all items with the given name.
    */
    int erase(const std::string &itemName)
    {
        return !current_section.empty() ? erase(current_section, itemName) : -1;
    }

    /**
    *  @brief erase the first item with the given name.
    */
    int erase_first(const std::string &section, const std::string &itemName)
    {
        string_multimap &mapTemp = ini_content.at(section);
        auto iter = mapTemp.find(itemName);
        if(iter != mapTemp.end())
        {
            mapTemp.erase(iter);
            return save_error_and_return(INIREADER_EXCEPTION_NONE);
        }
        else
        {
            return save_error_and_return(INIREADER_EXCEPTION_NOTEXIST);
        }
    }

    /**
    *  @brief erase the first item with the given name.
    */
    int erase_first(const std::string &itemName)
    {
        return !current_section.empty() ? erase_first(current_section, itemName) : -1;
    }

    /**
    *  @brief erase all items in the given section.
    */
    void erase_section(const std::string &section)
    {
        if(ini_content.find(section) == ini_content.end())
            return;
        erase_elements(ini_content.at(section));
        if(cached_section == section)
        {
            cached_section_content = ini_content.end();
            cached_section.erase();
        }
    }

    /**
    *  @brief erase all items in current section.
    */
    void erase_section()
    {
        if(!current_section.empty())
            erase_section(current_section);
    }

    /**
    *  @brief Remove a section from INI.
    */
    void remove_section(const std::string &section)
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
    void remove_section()
    {
        if(!current_section.empty())
            remove_section(current_section);
    }

    /**
    *  @brief Export the whole INI data structure into a string.
    */
    std::string to_string()
    {
        std::string content, itemVal;

        if(!parsed)
            return "";

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
    int to_file(const std::string &filePath)
    {
        return fileWrite(filePath, to_string(), true);
    }
};

#endif // INI_READER_H_INCLUDED

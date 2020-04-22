#include <string>

#include <rapidjson/writer.h>
#include <rapidjson/document.h>

#include "webget.h"
#include "ini_reader.h"
#include "logger.h"
#include "rapidjson_extra.h"

std::string buildGistData(std::string name, std::string content)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("description");
    writer.String("subconverter");
    writer.Key("public");
    writer.Bool(false);
    writer.Key("files");
    writer.StartObject();
    writer.Key(name.data());
    writer.StartObject();
    writer.Key("content");
    writer.String(content.data());
    writer.EndObject();
    writer.EndObject();
    writer.EndObject();
    return sb.GetString();
}

int uploadGist(std::string name, std::string path, std::string content, bool writeManageURL)
{
    INIReader ini;
    rapidjson::Document json;
    std::string token, id, username, retData, url;
    int retVal = 0;

    if(!fileExist("gistconf.ini"))
    {
        //std::cerr<<"gistconf.ini not found. Skipping...\n";
        writeLog(0, "gistconf.ini not found. Skipping...", LOG_LEVEL_ERROR);
        return -1;
    }

    ini.ParseFile("gistconf.ini");
    if(ini.EnterSection("common") != 0)
    {
        //std::cerr<<"gistconf.ini has incorrect format. Skipping...\n";
        writeLog(0, "gistconf.ini has incorrect format. Skipping...", LOG_LEVEL_ERROR);
        return -1;
    }

    token = ini.Get("token");
    if(!token.size())
    {
        //std::cerr<<"No token is provided. Skipping...\n";
        writeLog(0, "No token is provided. Skipping...", LOG_LEVEL_ERROR);
        return -1;
    }

    id = ini.Get("id");
    username = ini.Get("username");
    if(!path.size())
    {
        if(ini.ItemExist("path"))
            path = ini.Get(name, "path");
        else
            path = name;
    }

    if(!id.size())
    {
        //std::cerr<<"No gist id is provided. Creating new gist...\n";
        writeLog(0, "No Gist id is provided. Creating new Gist...", LOG_LEVEL_ERROR);
        retVal = webPost("https://api.github.com/gists", buildGistData(path, content), getSystemProxy(), {"Authorization: token " + token}, &retData);
        if(retVal != 201)
        {
            //std::cerr<<"Create new Gist failed! Return data:\n"<<retData<<"\n";
            writeLog(0, "Create new Gist failed! Return data:\n" + retData, LOG_LEVEL_ERROR);
            return -1;
        }
    }
    else
    {
        url = "https://gist.githubusercontent.com/" + username + "/" + id + "/raw/" + path;
        //std::cerr<<"Gist id provided. Modifying Gist...\n";
        writeLog(0, "Gist id provided. Modifying Gist...", LOG_LEVEL_INFO);
        if(writeManageURL)
            content = "#!MANAGED-CONFIG " + url + "\n" + content;
        retVal = webPatch("https://api.github.com/gists/" + id, buildGistData(path, content), getSystemProxy(), {"Authorization: token " + token}, &retData);
        if(retVal != 200)
        {
            //std::cerr<<"Modify gist failed! Return data:\n"<<retData<<"\n";
            writeLog(0, "Modify Gist failed! Return data:\n" + retData, LOG_LEVEL_ERROR);
            return -1;
        }
    }
    json.Parse(retData.data());
    GetMember(json, "id", id);
    if(json.HasMember("owner"))
        GetMember(json["owner"], "login", username);
    url = "https://gist.githubusercontent.com/" + username + "/" + id + "/raw/" + path;
    //std::cerr<<"Writing to Gist success!\nGenerator: "<<name<<"\nPath: "<<path<<"\nRaw URL: "<<url<<"\nGist owner: "<<username<<"\n";
    writeLog(0, "Writing to Gist success!\nGenerator: " + name + "\nPath: " + path + "\nRaw URL: " + url + "\nGist owner: " + username, LOG_LEVEL_INFO);

    ini.EraseSection();
    ini.Set("token", token);
    ini.Set("id", id);
    ini.Set("username", username);

    ini.SetCurrentSection(path);
    ini.EraseSection();
    ini.Set("type", name);
    ini.Set("url", url);

    ini.ToFile("gistconf.ini");
    return 0;
}

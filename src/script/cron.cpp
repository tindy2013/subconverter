#include <string>
#include <iostream>
#include <libcron/Cron.h>

#include "config/crontask.h"
#include "handler/interfaces.h"
#include "handler/multithread.h"
#include "handler/settings.h"
#include "server/webserver.h"
#include "utils/logger.h"
#include "utils/rapidjson_extra.h"
#include "utils/system.h"
#include "script_quickjs.h"

libcron::Cron cron;

struct script_info
{
    std::string name;
    time_t begin_time = 0;
    time_t timeout = 0;
};

int timeout_checker(JSRuntime *rt, void *opaque)
{
    script_info info = *static_cast<script_info*>(opaque);
    if(info.timeout != 0 && time(NULL) >= info.begin_time + info.timeout) /// timeout reached
    {
        writeLog(0, "Script '" + info.name + "' has exceeded timeout " + std::to_string(info.timeout) + ", terminate now.", LOG_LEVEL_WARNING);
        return 1;
    }
    return 0;
}

void refresh_schedule()
{
    cron.clear_schedules();
    for(const CronTaskConfig &x : global.cronTasks)
    {
        cron.add_schedule(x.Name, x.CronExp, [=](auto &)
        {
            qjs::Runtime runtime;
            qjs::Context context(runtime);
            try
            {
                script_runtime_init(runtime);
                script_context_init(context);
                defer(script_cleanup(context);)
                std::string proxy = parseProxy(global.proxyConfig);
                std::string script = fetchFile(x.Path, proxy, global.cacheConfig);
                if(script.empty())
                {
                    writeLog(0, "Script '" + x.Name + "' run failed: file is empty or not exist!", LOG_LEVEL_WARNING);
                    return;
                }
                script_info info;
                if(x.Timeout > 0)
                {
                    info.begin_time = time(NULL);
                    info.timeout = x.Timeout;
                    info.name = x.Name;
                    JS_SetInterruptHandler(JS_GetRuntime(context.ctx), timeout_checker, &info);
                }
                context.eval(script);
            }
            catch (qjs::exception)
            {
                script_print_stack(context);
            }
        });
    }
}

std::string list_cron_schedule(RESPONSE_CALLBACK_ARGS)
{
    auto &argument = request.argument;
    std::string token = getUrlArg(argument, "token");
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    if(token != global.accessToken)
    {
        response.status_code = 403;
        writer.Key("code");
        writer.Int(403);
        writer.Key("data");
        writer.String("Unauthorized");
        writer.EndObject();
        return sb.GetString();
    }
    writer.Key("code");
    writer.Int(200);
    writer.Key("tasks");
    writer.StartArray();
    for(const CronTaskConfig &x : global.cronTasks)
    {
        writer.StartObject();
        writer.Key("name");
        writer.String(x.Name.data());
        writer.Key("cronexp");
        writer.String(x.CronExp.data());
        writer.Key("path");
        writer.String(x.Path.data());
        writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
    return sb.GetString();
}

size_t cron_tick()
{
    return cron.tick();
}

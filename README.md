# subconverter

Utility to convert between various proxy subscription formats.

[![Build Status](https://travis-ci.com/tindy2013/subconverter.svg?branch=master)](https://travis-ci.com/tindy2013/subconverter)
[![GitHub tag (latest SemVer)](https://img.shields.io/github/tag/tindy2013/subconverter.svg)](https://github.com/tindy2013/subconverter/tags)
[![GitHub release](https://img.shields.io/github/release/tindy2013/subconverter.svg)](https://github.com/tindy2013/subconverter/releases)
[![GitHub license](https://img.shields.io/github/license/tindy2013/subconverter.svg)](https://github.com/tindy2013/subconverter/blob/master/LICENSE)

[Docker README](https://github.com/tindy2013/subconverter/blob/master/README-docker.md)

[中文文档](https://github.com/tindy2013/subconverter/blob/master/README-cn.md)

- [subconverter](#subconverter)
  - [Supported Types](#supported-types)
  - [Quick Usage](#quick-usage)
    - [Access Interface](#access-interface)
    - [Description](#description)
  - [Advanced Usage](#advanced-usage)
    - [Read Before Continue](#read-before-continue)
    - [Advanced Details](#advanced-details)
    - [Configuration File](#configuration-file)
    - [External Configuration File](#external-configuration-file)
  - [Auto Upload](#auto-upload)

## Supported Types

| Type         | As Source   | As Target     | Target Name |
| ------------ | :---------: | :-----------: | ----------- |
| Clash        |     ✔      |      ✔       | clash       |
| ClashR       |     ✔      |      ✔       | clashr      |
| Quantumult   |     ✔      |      ✔       | quan        |
| Quantumult X |     ✔      |      ✔       | quanx       |
| SS (SIP002)  |     ✔      |      ✔       | ss          |
| SS Android   |     ✔      |      ✔       | sssub       |
| SSD          |     ✔      |      ✔       | ssd         |
| SSR          |     ✔      |      ✔       | ssr         |
| Surfboard    |     ✔      |      ✔       | surfboard   |
| Surge 2      |     ✔      |      ✔       | surge&ver=2 |
| Surge 3      |     ✔      |      ✔       | surge&ver=3 |
| Surge 4      |     ✔      |      ✔       | surge&ver=4 |
| V2Ray        |     ✔      |      ✔       | v2ray       |

**Notice**：Shadowrocket users should use `ss`, `ssr` or `v2ray` as target.

---

## Quick Usage

> Using default groups and rulesets configuration directly, without changing any settings

### Access Interface

```TXT
http://127.0.0.1:25500/sub?target=%TARGET%&url=%URL%&config=%CONFIG%
```

### Description

| Argument | Needed | Example | Explanation |
| -------  | :----: | :------ | ----------- |
| target   | Yes    | clash   | Target subscription type. Acquire from Target Name in [Supported Types](#Supported_Types). |
| url      | Yes    | https%3A%2F%2Fwww.xxx.com | Subscription to convert. Supports URLs and file paths. Process with [URLEncode](https://www.urlencoder.org/) first. |
| config   | No     | https%3A%2F%2Fwww.xxx.com | External configuration file path. Supports URLs and file paths. Process with [URLEncode](https://www.urlencoder.org/) first. More examples can be found in [this](https://github.com/lzdnico/subconverteriniexample) repository. Default is to load configurations from `pref.ini`. |

If you need to merge two or more subscription, you should connect them with '|' before the URLEncode process.

Example:

```TXT
You have 2 subscriptions and you want to merge them and generate a Clash subscription:
1. https://dler.cloud/subscribe/ABCDE?clash=vmess
2. https://rich.cloud/subscribe/ABCDE?clash=vmess

First use '|' to separate 2 subscriptions:
https://dler.cloud/subscribe/ABCDE?clash=vmess|https://rich.cloud/subscribe/ABCDE?clash=vmess

Then process it with URLEncode to get %URL%:
https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess%7Chttps%3A%2F%2Frich.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

Then fill %TARGET% and %URL% in Access Interface with actual values:
http://127.0.0.1:25500/sub?target=clash&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess%7Chttps%3A%2F%2Frich.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

Finally subscribe this link in Clash and you are done!
```

---

## Advanced Usage

> If you are not satisfied with the default groups and rulesets, you can try out advanced usage.
> Customize more settings in Access Interface and `pref.ini` to satisfy various needs.

### Read Before Continue

It is strongly recommended to read the following articles before continuing:

1. Related to `pref.ini`: [INI file](https://en.wikipedia.org/wiki/INI_file)
1. Related to `Clash` configurations: [YAML Syntax](https://en.wikipedia.org/wiki/YAML#Syntax)
1. Often needed: [Learn Regular Expression](https://github.com/ziishaned/learn-regex/blob/master/README.md)
1. When you want to write an ISSUE: [How To Ask Questions The Smart Way](http://www.catb.org/~esr/faqs/smart-questions.html)

Subconverter only guaranteed to work with default configurations.

### Advanced Details

#### Access Interface

```txt
http://127.0.0.1:25500/sub?target=%TARGET%&url=%URL%&emoji=%EMOJI%····
```

#### Description

| Argument | Needed  | Example | Explanation |
| -------- | :----:  | :--------------- | :------------------------ |
| target   |  Yes    | quan    | Target subscription type. Acquire from Target Name in [Supported Types](#Supported_Types). |
| url      |  No     | https%3A%2F%2Fwww.xxx.com | Subscription to convert. Supports URLs, data URIs, and file paths. Process with [URLEncode](https://www.urlencoder.org/) first. **Not needed ONLY WHEN YOU HAVE SET `default_urls` IN `pref.ini`. |
| config   |  No     | https%3A%2F%2Fwww.xxx.com | External configuration file path. Supports URLs and file paths. Process with [URLEncode](https://www.urlencoder.org/) first. More examples can be found in [this](https://github.com/lzdnico/subconverteriniexample) repository. Default is to load configurations from `pref.ini`. |
| upload   |  No     | true / false  | Upload generated configuration to `Gist repository`. `gistconf.ini` must be filled before uploading. Default is `false`. |
| upload_path |  No     | MySS.yaml  | File name when uploaded to `Gist`. Process with [URLEncode](https://www.urlencoder.org/) first.    |
| emoji    |  No     | true / false  | Adding Emoji to node remarks. Default is `true`.  |
| group    |  No     | MySS  | Set a custom group for generated configuration. Often needed in SSD/SSR subscription.  |
| tfo      |  No     | true / false  | Enable TCP Fast Open for all nodes. Default is `false`.  |
| udp      |  No     | true / false  | Enable UDP for all nodes. Default is `false`.  |
| scv      |  No     | true / false  | Enable Skip Cert Verify for all nodes. Default is `false`.  |
| list     |  No     | true / false  | Generate Surge Node List or Clash Proxy Provider. Default is `false`.  |
| sort     |  No     | true / false  | Sort nodes in alphabetical order. Default is `false`.  |
| include  |  No     | See `include_remarks`  | Exclude nodes which remarks match the following patterns. Supports regular expression. Process with [URLEncode](https://www.urlencoder.org/) first. **WILL OVERRIDE THE SAME SETTING IN `pref.ini`**  |
| exclude  |  No     | See `exclude_remarks`  | Only include nodes which remarks match the following patterns. Supports regular expression. Process with [URLEncode](https://www.urlencoder.org/) first. **WILL OVERRIDE THE SAME SETTING IN `pref.ini`**  |

Example: 

```TXT
You have the following subscription: `https://dler.cloud/subscribe/ABCDE?clash=vmess`, and you want to convert it to Surge 4 subscription, set UDP and TFO to enabled,
add Emoji to node remarks and filter out unused nodes named "剩余流量：1024G" and "官网地址：dler.cloud".

First find all needed arguments: 
target=surge, ver=4,  tfo=true, udp=true, emoji=true, exclude=(流量|官网)
url=https://dler.cloud/subscribe/ABCDE?clash=vmess

Then process any argument that requires URLEncode: 
exclude=%28%E6%B5%81%E9%87%8F%7C%E5%AE%98%E7%BD%91%29
url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

Then merge everything into a single URL: 
http://127.0.0.1:25500/sub?surge&ver=4&tfo=true&udp=true&emoji=true&exclude=%28%E6%B5%81%E9%87%8F%7C%E5%AE%98%E7%BD%91%29&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

Finally subscribe this link in Surge and you are done!
```
### Configuration File

> Check comments inside [pref.ini](https://github.com/tindy2013/subconverter/blob/master/base/pref.ini) for more information.

### External Configuration File

> Most settings works the same as the ones with the same name inside `pref.ini`, you can check [the example configuration file](https://github.com/tindy2013/subconverter/blob/master/base/config/example_external_config.ini) and comments inside [pref.ini](https://github.com/tindy2013/subconverter/blob/master/base/pref.ini) for more information.

Any setting defined in the external configuration file will **override** the ones from `pref.ini`.

For example, if you have the following lines inside the external configuration file:

```
emoji=(流量|时间|应急),🏳️‍🌈
emoji=阿根廷,🇦🇷
```

Then instead of the Emojis defined in `pref.ini`, the program will only use the newly defined ones.

## Auto Upload

> Upload Gist automatically

Add a [Personal Access Token](https://github.com/settings/tokens/new) into [gistconf.ini](./gistconf.ini) in the root directory, then add `&upload=true` to the local subscription link, then when you access this link, the program will automatically update the content to Gist repository.

Example:

```ini
[common]
;uncomment the following line and enter your token to enable upload function
token = xxxxxxxxxxxxxxxxxxxxxxxx(Your Personal Access Token)
```

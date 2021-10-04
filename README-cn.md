# subconverter

在各种订阅格式之间进行转换的实用程序.

[![Build Status](https://github.com/tindy2013/subconverter/actions/workflows/build.yml/badge.svg)](https://github.com/tindy2013/subconverter/actions)
[![GitHub tag (latest SemVer)](https://img.shields.io/github/tag/tindy2013/subconverter.svg)](https://github.com/tindy2013/subconverter/tags)
[![GitHub release](https://img.shields.io/github/release/tindy2013/subconverter.svg)](https://github.com/tindy2013/subconverter/releases)
[![GitHub license](https://img.shields.io/github/license/tindy2013/subconverter.svg)](https://github.com/tindy2013/subconverter/blob/master/LICENSE)

* * *

## 新增内容

2021/10/1

-   新增 [配置文件](#配置文件) 中 `[advanced]` 部分的说明
-   修改调整文档中的多处描述
-   更换文档中失效的外部链接

<details>
<summary><b>更新历史</b></summary>
2020/12/9

-   新增 [特别用法](#特别用法) 中 [规则转换](#规则转换) 的说明
-   修改 [配置文件](#配置文件) 中的 `clash_proxy_group` 为 `proxy_group` ，并增加修改描述与示例
-   修改 [配置文件](#配置文件) 中 `[ruleset]` 部分的 `surge_ruleset` 为 `ruleset ` ，并增加修改示例
-   修改 [外部配置](#外部配置) 中 `surge_ruleset` 为 `ruleset ` 
-   新增 [外部配置](#外部配置) 中 `add_emoji` 和 `remove_old_emoji` 
-   修改 [外部配置](#外部配置) 中 `proxy_group` 和  `ruleset ` 的描述与示例
-   调整 [简易用法](#简易用法) 与 [进阶用法](#进阶用法) 中的部分描述
-   更换文档中失效的外部链接

2020/11/20

-   新增 [支持类型](#支持类型) 中 `mixed` & `auto` 参数
-   新增 [进阶链接](#进阶链接) 中多个调用参数的说明
-   新增 [配置文件](#配置文件) 中 `[userinfo]` 部分的描述
-   新增 [配置文件](#配置文件) 中 `[common]`&`[node_pref]`&`[server]` 中多个参数的描述
-   修改 [进阶链接](#进阶链接) 中 `url` 参数的说明

2020/04/29

-   新增 [配置文件](#配置文件) 指定默认外部配置文件
-   新增 [配置文件](#配置文件) 中 `[aliases]` 参数的描述
-   新增 [模板功能](#模板功能) 用于直接渲染的 `/render` 接口的描述
-   修改 [支持类型](#支持类型) 中类 TG 类型节点的描述
-   调整 模板介绍 为 [模板功能](#模板功能)

2020/04/04

-   新增 [模板介绍](#模板介绍) 用于对所引用的 `base` 基础模板进行高度个性化自定义
-   新增 [配置文件](#配置文件) 中 `[template]` 参数的描述
-   新增 [外部配置](#外部配置) 中 `[template]` 参数的描述
-   新增 [本地生成](#本地生成) 用于在本地生成具体的配置文件
-   新增 [支持类型](#支持类型) 中 `mellow` & `trojan` 参数
-   新增 [进阶链接](#进阶链接) 中 `new_name` 参数的描述
-   新增 [配置文件](#配置文件) 中 `append_sub_userinfo` `clash_use_new_field_name` 参数的描述
-   调整 [说明目录](#说明目录) 层次

2020/03/02

-   新增 [进阶链接](#进阶链接) 中关于 `append_type` `append_info` `expand` `dev_id` `interval` `strict` 等参数的描述

</details>

* * *

## 说明目录

-   [subconverter](#subconverter)

    -   [新增内容](#新增内容)

    -   [说明目录](#说明目录)

    -   [支持类型](#支持类型)

    -   [简易用法](#简易用法)

        -   [调用地址](#调用地址)
        -   [调用说明](#调用说明)
        -   [简易转换](#简易转换)

    -   [进阶用法](#进阶用法)

        -   [阅前提示](#阅前提示)

        -   [进阶链接](#进阶链接)

            -   [调用地址 (进阶)](#调用地址-进阶)
            -   [调用说明 (进阶)](#调用说明-进阶)

        -   [配置档案](#配置档案)

            -   [调用地址 (档案)](#调用地址-档案)
            -   [调用说明 (档案)](#调用说明-档案)

        -   [配置文件](#配置文件)

        -   [外部配置](#外部配置)

        -   [模板功能](#模板功能)

            -   [模板调用](#模板调用)
            -   [直接渲染](#直接渲染)

    -   [特别用法](#特别用法)

        -   [本地生成](#本地生成)

        -   [自动上传](#自动上传)

        -   [规则转换](#规则转换)

            -   [调用地址 (规则转换)](#调用地址-规则转换)
            -   [调用说明 (规则转换)](#调用说明-规则转换)

## 支持类型

| 类型                     | 作为源类型 | 作为目标类型 | 参数             |
| ---------------------- | :---: | :----: | -------------- |
| Clash                  |   ✓   |    ✓   | clash          |
| ClashR                 |   ✓   |    ✓   | clashr         |
| Quantumult (完整配置)      |   ✓   |    ✓   | quan           |
| Quantumult X (完整配置)    |   ✓   |    ✓   | quanx          |
| Loon                   |   ✓   |    ✓   | loon           |
| Mellow                 |   ✓   |    ✓   | mellow         |
| SS (SIP002)            |   ✓   |    ✓   | ss             |
| SS (软件订阅/SIP008)       |   ✓   |    ✓   | sssub          |
| SSD                    |   ✓   |    ✓   | ssd            |
| SSR                    |   ✓   |    ✓   | ssr            |
| Surfboard              |   ✓   |    ✓   | surfboard      |
| Surge 2                |   ✓   |    ✓   | surge&ver=2    |
| Surge 3                |   ✓   |    ✓   | surge&ver=3    |
| Surge 4                |   ✓   |    ✓   | surge&ver=4    |
| Trojan                 |   ✓   |    ✓   | trojan         |
| V2Ray                  |   ✓   |    ✓   | v2ray          |
| 类 TG 代理的 HTTP/Socks 链接 |   ✓   |    ×   | 仅支持 `&url=` 调用 |
| Mixed                  |   ×   |    ✓   | mixed          |
| Auto                   |   ×   |    ✓   | auto           |

注意：

1.  Shadowrocket 用户可以使用 `ss`、`ssr` 、 `v2ray` 以及 `mixed` 参数

2.  类 TG 代理的 HTTP/Socks 链接由于没有命名设定，所以可以在后方插入`&remarks=`进行命名，同时也可以插入 `&group=` 设置组别名称，以上两个参数需要经过 [URLEncode](https://www.urlencoder.org/) 处理，例如

    -   tg://http?server=1.2.3.4&port=233&user=user&pass=pass&remarks=Example&group=xxx
    -   <https://t.me/http?server=1.2.3.4&port=233&user=user&pass=pass&remarks=Example&group=xxx>

3.  目标类型为 `mixed` 时，会输出所有支持的节点的单链接组成的普通订阅（Base64编码）

4.  目标类型为 `auto` 时，会根据请求的 `User-Agent` 自动判断输出的目标类型，匹配规则可参见 [此处](https://github.com/tindy2013/subconverter/blob/master/src/handler/interfaces.cpp#L121) （该链接有可能因为代码修改而不能准确指向相应的代码）

* * *

## 简易用法

> 即生成的订阅使用 **默认设置**

### 调用地址

```txt
http://127.0.0.1:25500/sub?target=%TARGET%&url=%URL%&config=%CONFIG%
```

### 调用说明

| 调用参数   | 必要性 | 示例                        | 解释                                                                                                                  |
| ------ | :-: | :------------------------ | ------------------------------------------------------------------------------------------------------------------- |
| target |  必要 | surge&ver=4               | 指想要生成的配置类型，详见上方 [支持类型](#支持类型) 中的参数                                                                                  |
| url    |  必要 | https%3A%2F%2Fwww.xxx.com | 指机场所提供的订阅链接或代理节点的分享链接，需要经过 [URLEncode](https://www.urlencoder.org/) 处理                                              |
| config |  可选 | https%3A%2F%2Fwww.xxx.com | 指 外部配置 的地址 (包含分组和规则部分)，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，详见 [外部配置](#外部配置) ，当此参数不存在时使用 程序的主程序目录中的配置文件 |

运行 subconverter 主程序后，按照 [调用说明](#调用说明) 的对应内容替换即可得到一份使用**默认设置**的订阅。

由于此部分篇幅较长，点击下方条目即可展开详解：

<details>
<summary><b>处理单份订阅</b></summary>

如果你需要将一份 Surge 订阅转换成 Clash 的订阅, 可以按以下操作：

```txt
有以下一个订阅，且想转换成 Clash 的订阅:
1. https://dler.cloud/subscribe/ABCDE?surge=ss

首先将订阅通过 URLEncode 后可以得到:
https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fsurge%3Dss

然后将想要的 %TARGET% (即 Clash) 和上一步所得到的 %URL% 填入调用地址中:
http://127.0.0.1:25500/sub?target=clash&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fsurge%3Dss

最后将该链接填写至 Clash 的订阅处就大功告成了。
```

</details>

<details>
<summary><b>处理多份订阅</b></summary>

如果你需要将多个订阅合成一份, 则要在上方所提及的 URLEncode 之前使用 '|' 来分隔链接, 可以按以下操作：

```txt
有以下两个订阅，且想合并转换成 Clash 的订阅:
1. https://dler.cloud/subscribe/ABCDE?clash=vmess
2. https://rich.cloud/subscribe/ABCDE?clash=vmess

首先使用 '|' 将两个订阅分隔开:
https://dler.cloud/subscribe/ABCDE?clash=vmess|https://rich.cloud/subscribe/ABCDE?clash=vmess

接着通过 URLEncode 后可以得到:
https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess%7Chttps%3A%2F%2Frich.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

然后将想要的 %TARGET% (即 Clash) 和上一步所得到的 %URL% 填入调用地址中:
http://127.0.0.1:25500/sub?target=clash&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess%7Chttps%3A%2F%2Frich.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

最后将该链接填写至 Clash 的订阅处就大功告成了。
```

</details>

<details>
<summary><b>处理单份链接</b></summary>

如果你需要将自建的一条 SS 的 SIP002 链接转换成 Clash 的订阅, 可以按以下操作：

```txt
有以下自建的一条 SS 的 SIP002 链接，且想转换成 Clash 的订阅:
1. ss://YWVzLTEyOC1nY206dGVzdA==@192.168.100.1:8888#Example1

首先将订阅通过 URLEncode 后可以得到:
ss%3A%2F%2FYWVzLTEyOC1nY206dGVzdA%3D%3D%40192%2E168%2E100%2E1%3A8888%23Example1

然后将想要的 %TARGET% (即 Clash) 和上一步所得到的 %URL% 填入调用地址中:
http://127.0.0.1:25500/sub?target=clash&url=ss%3A%2F%2FYWVzLTEyOC1nY206dGVzdA%3D%3D%40192%2E168%2E100%2E1%3A8888%23Example1

最后将该链接填写至 Clash 的订阅处就大功告成了。
```

</details>

<details>
<summary><b>处理多份链接</b></summary>

如果你需要将多个链接合成一份, 则要在上方所提及的 URLEncode 之前使用 '|' 来分隔链接, 可以按以下操作：

```txt
有以下两个链接，且想合并转换成 Clash 的订阅:
1. ss://YWVzLTEyOC1nY206dGVzdA==@192.168.100.1:8888#Example1
2. vmess://eyJ2IjoiMiIsInBzIjoidm1lc3MtcHJveHkxIiwiYWRkIjoiZXhhbXBsZS5jb20iLCJwb3J0Ijo0NDMsInR5cGUiOiIiLCJpZCI6IjEyMzQ1Njc4LWFiY2QtMTIzNC0xMjM0LTQ3ZmZjYTBjZTIyOSIsImFpZCI6NDQzLCJuZXQiOiJ3cyIsInBhdGgiOiIvdjIiLCJob3N0IjoiZXhhbXBsZS5jb20iLCJ0bHMiOiJ0bHMifQ==

首先使用 '|' 将两个链接分隔开:
ss://YWVzLTEyOC1nY206dGVzdA==@192.168.100.1:8888#Example1|vmess://eyJ2IjoiMiIsInBzIjoidm1lc3MtcHJveHkxIiwiYWRkIjoiZXhhbXBsZS5jb20iLCJwb3J0Ijo0NDMsInR5cGUiOiIiLCJpZCI6IjEyMzQ1Njc4LWFiY2QtMTIzNC0xMjM0LTQ3ZmZjYTBjZTIyOSIsImFpZCI6NDQzLCJuZXQiOiJ3cyIsInBhdGgiOiIvdjIiLCJob3N0IjoiZXhhbXBsZS5jb20iLCJ0bHMiOiJ0bHMifQ==

接着通过 URLEncode 后可以得到:
ss%3A%2F%2FYWVzLTEyOC1nY206dGVzdA%3D%3D%40192%2E168%2E100%2E1%3A8888%23Example1%7Cvmess%3A%2F%2FeyJ2IjoiMiIsInBzIjoidm1lc3MtcHJveHkxIiwiYWRkIjoiZXhhbXBsZS5jb20iLCJwb3J0Ijo0NDMsInR5cGUiOiIiLCJpZCI6IjEyMzQ1Njc4LWFiY2QtMTIzNC0xMjM0LTQ3ZmZjYTBjZTIyOSIsImFpZCI6NDQzLCJuZXQiOiJ3cyIsInBhdGgiOiIvdjIiLCJob3N0IjoiZXhhbXBsZS5jb20iLCJ0bHMiOiJ0bHMifQ%3D%3D

然后将想要的 %TARGET% (即 Clash) 和上一步所得到的 %URL% 填入调用地址中:
http://127.0.0.1:25500/sub?target=clash&url=ss%3A%2F%2FYWVzLTEyOC1nY206dGVzdA%3D%3D%40192%2E168%2E100%2E1%3A8888%23Example1%7Cvmess%3A%2F%2FeyJ2IjoiMiIsInBzIjoidm1lc3MtcHJveHkxIiwiYWRkIjoiZXhhbXBsZS5jb20iLCJwb3J0Ijo0NDMsInR5cGUiOiIiLCJpZCI6IjEyMzQ1Njc4LWFiY2QtMTIzNC0xMjM0LTQ3ZmZjYTBjZTIyOSIsImFpZCI6NDQzLCJuZXQiOiJ3cyIsInBhdGgiOiIvdjIiLCJob3N0IjoiZXhhbXBsZS5jb20iLCJ0bHMiOiJ0bHMifQ%3D%3D

最后将该链接填写至 Clash 的订阅处就大功告成了。
```

</details>

### 简易转换

当机场提供的 Surge 配置足以满足需求，但额外需要使用 Clash 订阅时，此时可以使用以下方式进行转换

```txt
http://127.0.0.1:25500/surge2clash?link=Surge的订阅链接
```

此处 `Surge的订阅链接`**不需要进行URLEncode**，且**无需任何额外配置**。

* * *

## 进阶用法

> 在不满足于本程序所提供的默认规则或者对应的分组时，可以考虑尝试进阶用法
>
> 即 对 `调用地址` 甚至程序目录下的 `配置文件` 进行个性化的编辑以满足不同的需求

### 阅前提示

在进行下一步操作前，十分推荐您阅读以下内容：

1.  与 调用地址 相关的：[什么是URL？](https://developer.mozilla.org/zh-CN/docs/Learn/Common_questions/What_is_a_URL)
2.  与 配置文件 相关的：[INI 语法介绍](https://zh.wikipedia.org/wiki/INI%E6%96%87%E4%BB%B6) 、 [YAML 语法介绍](https://zh.wikipedia.org/wiki/YAML#%E8%AA%9E%E6%B3%95) 以及  [TOML 语法介绍](https://toml.io/cn/v1.0.0)
3.  与 `Clash` 配置相关的：[YAML 语法介绍](https://zh.wikipedia.org/wiki/YAML#%E8%AA%9E%E6%B3%95) 以及 [官方文档](https://github.com/Dreamacro/clash/wiki/configuration)
4.  与 `模板` 配置相关的：[INJA 语法介绍](https://github.com/pantor/inja)
5.  会经常涉及到的： [正则表达式入门](https://github.com/ziishaned/learn-regex/blob/master/translations/README-cn.md)
6.  当遇到问题需要提交 ISSUE 时的：[提问的智慧](https://github.com/ryanhanwu/How-To-Ask-Questions-The-Smart-Way/blob/master/README-zh_CN.md)

当您尝试进行进阶操作时，即默认您有相关的操作能力，本程序仅保证在默认配置文件下能够正常运行。

### 进阶链接

#### 调用地址 (进阶)

```txt
http://127.0.0.1:25500/sub?target=%TARGET%&url=%URL%&emoji=%EMOJI%····
```

#### 调用说明 (进阶)

| 调用参数          | 必要性 | 示例                        | 解释                                                                                                                                                                                                          |
| ------------- | :-: | :------------------------ | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| target        |  必要 | surge&ver=4               | 指想要生成的配置类型，详见上方 [支持类型](#支持类型) 中的参数                                                                                                                                                                          |
| url           |  可选 | https%3A%2F%2Fwww.xxx.com | 指机场所提供的订阅链接或代理节点的分享链接，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，**可选的前提是在 `default_url` 中进行指定**。也可以使用 data URI。可使用 `tag:xxx,https%3A%2F%2Fwww.xxx.com` 指定该订阅的所有节点归属于`xxx`分组，用于配置文件中的`!!GROUP=XXX` 匹配 |
| group         |  可选 | MySS                      | 用于设置该订阅的组名，多用于 SSD/SSR                                                                                                                                                                                      |
| upload_path   |  可选 | MySS.yaml                 | 用于将生成的订阅文件上传至 `Gist` 后的名称，需要经过 [URLEncode](https://www.urlencoder.org/) 处理                                                                                                                                  |
| include       |  可选 | 详见下文中 `include_remarks`   | 指仅保留匹配到的节点，支持正则匹配，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，会覆盖配置文件里的设置                                                                                                                              |
| exclude       |  可选 | 详见下文中 `exclude_remarks`   | 指排除匹配到的节点，支持正则匹配，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，会覆盖配置文件里的设置                                                                                                                               |
| config        |  可选 | https%3A%2F%2Fwww.xxx.com | 指 外部配置 的地址 (包含分组和规则部分)，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，详见 [外部配置](#外部配置) ，当此参数不存在时使用 主程序目录中的配置文件                                                                                            |
| dev_id        |  可选 | 92DSAFA                   | 用于设置 QuantumultX 的远程设备 ID, 以在某些版本上开启远程脚本                                                                                                                                                                    |
| filename      |  可选 | MySS                      | 指定所生成订阅的文件名，可以在 Clash For Windows 等支持文件名的软件中显示出来                                                                                                                                                            |
| interval      |  可选 | 43200                     | 用于设置托管配置更新间隔，确定配置将更新多长时间，单位为秒                                                                                                                                                                               |
| rename        |  可选 | 详见下文中 `rename`            | 用于自定义重命名，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，会覆盖配置文件里的设置                                                                                                                                       |
| filter_script |  可选 | 详见下文中 `filter_script`     | 用于自定义筛选节点的js代码，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，会覆盖配置文件里的设置。出于安全考虑，链接需包含正确的 `token` 参数，才会应用该设置                                                                                              |
| strict        |  可选 | true / false              | 如果设置为 true，则 Surge 将在上述间隔后要求强制更新                                                                                                                                                                            |
| upload        |  可选 | true / false              | 用于将生成的订阅文件上传至 `Gist`，需要填写`gistconf.ini`，默认为 false (即不上传) ,详见 [自动上传](#自动上传)                                                                                                                                  |
| emoji         |  可选 | true / false              | 用于设置节点名称是否包含 Emoji，默认为 true                                                                                                                                                                                 |
| add_emoji     |  可选 | true / false              | 用于在节点名称前加入 Emoji，默认为 true                                                                                                                                                                                   |
| remove_emoji  |  可选 | true / false              | 用于设置是否删除节点名称中原有的 Emoji，默认为 true                                                                                                                                                                             |
| append_type   |  可选 | true / false              | 用于在节点名称前插入节点类型，如 `[SS]`,`[SSR]`等                                                                                                                                                                               |
| tfo           |  可选 | true / false              | 用于开启该订阅链接的 TCP Fast Open，默认为 false                                                                                                                                                                          |
| udp           |  可选 | true / false              | 用于开启该订阅链接的 UDP，默认为 false                                                                                                                                                                                    |
| list          |  可选 | true / false              | 用于输出 Surge Node List 或者 Clash Proxy Provider 或者 Quantumult (X) 的节点订阅 或者 解码后的 SIP002                                                                                                                         |
| sort          |  可选 | true / false              | 用于对输出的节点或策略组按节点名进行再次排序，默认为 false                                                                                                                                                                            |
| sort_script   |  可选 | 详见下文 `sort_script`        | 用于自定义排序的js代码，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，会覆盖配置文件里的设置。出于安全考虑，链接需包含正确的 `token` 参数，才会应用该设置                                                                                                |
| script        |  可选 | true / false              | 用于生成Clash Script，默认为 false                                                                                                                                                                                  |
| insert        |  可选 | true / false              | 用于设置是否将配置文件中的 `insert_url` 插入，默认为 true                                                                                                                                                                      |
| scv           |  可选 | true / false              | 用于关闭 TLS 节点的证书检查，默认为 false                                                                                                                                                                                  |
| fdn           |  可选 | true / false              | 用于过滤目标类型不支持的节点，默认为 true                                                                                                                                                                                     |
| expand        |  可选 | true / false              | 用于在 API 端处理或转换 Surge, QuantumultX, Clash 的规则列表，即是否将规则全文置入订阅中，默认为 true，设置为 false 则不会将规则全文写进订阅                                                                                                                |
| append_info   |  可选 | true / false              | 用于输出包含流量或到期信息的节点, 默认为 true，设置为 false 则取消输出                                                                                                                                                                  |
| prepend       |  可选 | true / false              | 用于设置插入 `insert_url` 时是否插入到所有节点前面，默认为 true                                                                                                                                                                   |
| classic       |  可选 | true / false              | 用于设置是否生成 Clash classical rule-provider                                                                                                                                                                      |
| tls13         |  可选 | true / false              | 用于设置是否为节点增加tls1.3开启参数                                                                                                                                                                                       |
| new_name      |  可选 | true / false              | 如果设置为 true，则将启用 Clash 的新组名称 (proxies, proxy-groups, rules)                                                                                                                                                  |

举个例子：

```txt
有订阅 `https://dler.cloud/subscribe/ABCDE?clash=vmess`，想转换成 Surge 4 的订阅，且需要开启 TFO 和 UDP
顺便再给节点名加上 EMOJI 同时排除掉订阅中显示流量和官网的节点（节点名为"剩余流量：1024G"，"官网地址：dler.cloud"）

首先确认需要用到的参数：
target=surge&ver=4 、 tfo=true 、 udp=true 、 emoji=true 、exclude=(流量|官网)
url=https://dler.cloud/subscribe/ABCDE?clash=vmess

然后将需要 URLEncode 的部分进行处理：
exclude=%28%E6%B5%81%E9%87%8F%7C%E5%AE%98%E7%BD%91%29
url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

接着将所有元素进行拼接：
http://127.0.0.1:25500/sub?target=surge&ver=4&tfo=true&udp=true&emoji=true&exclude=%28%E6%B5%81%E9%87%8F%7C%E5%AE%98%E7%BD%91%29&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

最后将该链接填写至 Surge 的订阅处就大功告成了。
```

### 配置档案

> 当通过上述 [进阶链接](#进阶链接) 配置好订阅链接后，通常会使得链接十分冗长和难以记忆，此时可以考虑使用配置档案。

此功能暂时**仅能读取本地文件**

#### 调用地址 (档案)

```txt
http://127.0.0.1:25500/getprofile?name=%NAME%&token=%TOKEN%
```

#### 调用说明 (档案)

| 调用参数  | 必要性 | 示例                        | 解释                                                                             |
| ----- | :-: | :------------------------ | :----------------------------------------------------------------------------- |
| name  |  必要 | profiles/formyairport.ini | 指配置档案的存储位置(可使用基于**pref 配置文件**的相对位置)                                            |
| token |  必要 | passwd                    | 为了安全考虑**必须设置token**（详见 [配置文件](#配置文件) 中 `[common] 部分` 对 `api_access_token` 的描述） |

应当注意的是，此处文件内的参数**无需进行 URLEncode**，且此处的 `token` 与 `api_mode` 的状态无关。

在程序目录内的任意位置创建一个新的文档文件（推荐保存至 `profiles` 文件夹内，以使整洁目录及便于后续维护），如 `formyairport.ini`，并仿照 [示例文档](https://github.com/tindy2013/subconverter/blob/master/base/profiles/example_profile.ini) 根据配置好的参数填写进去即可。

<details>
<summary>举个例子：</summary>

以上述 [进阶链接](#进阶链接) 的例子而言，`formyairport.ini` 内的内容应当是：

```txt
[Profile]
url=https://dler.cloud/subscribe/ABCDE?clash=vmess
target=surge
surge_ver=4
tfo=true
udp=true
emoji=true
exclude=(流量|官网)
```

在编辑并保存好 `formyairport.ini` 后，即可使用 `http://127.0.0.1:25500/getprofile?name=profiles/formyairport.ini&token=passwd` 进行调用。

</details>

### 配置文件

> 关于 subconverter 主程序目录中 `pref.ini` 文件的解释，其余格式的配置文件不再赘述，与之相仿。

注：本部分内容以本程序中的 [`pref.example.ini`](https://github.com/tindy2013/subconverter/blob/master/base/pref.example.ini) 或 [`pref.example.yml`](https://github.com/tindy2013/subconverter/blob/master/base/pref.example.yml) 或 [`pref.example.toml`](https://github.com/tindy2013/subconverter/blob/master/base/pref.example.toml) 为准，本文档可能由于更新不及时，内容不适用于新版本。

加载配置文件时会按照`pref.toml`、`pref.yml`、`pref.ini`的优先级顺序加载优先级高的配置文件

由于此部分篇幅较长，点击下方条目即可展开详解：

<details>
<summary><b>[common] 部分</b></summary>

> 该部分主要涉及到的内容为 **全局的节点排除或保留** 、**各配置文件的基础**
>
> 其他设置项目可以保持默认或者在知晓作用的前提下进行修改

1.  **api_mode**

    > API 模式，设置为 true 以防止直接加载本地订阅或直接提供本地文件，若访问这些内容则需要接上 `&token=`。（多用于部署公共订阅转换服务时）

    -   当值为 `false` 时, 每次更新配置都会读取 主程序目录中的配置文件 , 为 `true` 时则仅启动时读取。

2.  **api_access_token**

    > 用于访问相对隐私的接口（如 `/getprofile`）

    -   例如:

        ```ini
        api_access_token=passwd
        ```

3.  **default_url**

    > 无 %URL% 参数时，默认加载的订阅链接， **不需要 URLEncode**。
    >
    > 如果有多个链接，仍然需要使用 "|" 分隔，支持`文件`/`url`

    -   例如:

        ```ini
        default_url=https://dler.cloud/subscribe/ABCDE?clash=vmess
        ```

    -   解释：

        ```txt
        此时订阅链接:
        http://127.0.0.1:25500/sub?target=clash
        等同于:
        http://127.0.0.1:25500/sub?target=clash&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess
        ```

4.  **enable_insert**

    > 设置是否为输出的订阅添加 `insert_url` 中所有的节点

    -   当值为 `true` 时, 会在输出的订阅中添加 `insert_url` 中所有的节点, 为 `false` 时不添加。

5.  **insert_url**

    > 当 `enable_insert` 的值为 `true` 时，无论是否具有 %URL% 参数时，都会在添加订阅前加入的节点， **不需要 URLEncode**。
    >
    > 如果有多个节点，仍然需要使用 "|" 分隔，支持 `单个节点`/`订阅链接`
    >
    > 支持 SS/SSR/Vmess 以及类 TG 代理的 HTTP/Socks 链接

    -   例如:

        ```ini
        insert_url=ss://Y2hhY2hhMjAtaWV0Zi1wb2x5MTMwNTpwYXNzd29yZA@www.example.com:1080#Example
        insert_url=ss://Y2hhY2hhMjAtaWV0Zi1wb2x5MTMwNTpwYXNzd29yZA@www.example.com:1080#Example
        ```

6.  **prepend_insert_url**

    > 设置为输出的订阅添加 `insert_url` 中的节点时是否添加至所有节点前方

    -   当值为 `true` 时, 会在输出的订阅中所有节点的前方添加 `insert_url` 中所有的节点, 为 `false` 时在后方添加。

7.  **exclude_remarks**

    > 排除匹配到的节点，支持正则匹配

    -   例如:

        ```ini
        exclude_remarks=(到期|剩余流量|时间|官网|产品|平台)
        ```

8.  **include_remarks**

    > 仅保留匹配到的节点，支持正则匹配

    -   例如:

        ```ini
        include_remarks=(?<=美).*(BGP|GIA|IPLC)
        ```

9.  **enable_filter**

    > 设置为所有节点使用自定义的js代码进行筛选

    -   当值为 `true` 时, 为所有节点使用自定义的js代码进行筛选, 为 `false` 时不使用。

10. **filter_script**

    > 为所有节点使用自定义的js函数进行筛选
    >
    > 可设置为js代码内容，也可为本地js文件的路径
    >
    > js函数包括一个参数，即一个节点，函数返回为true时保留该节点，返回为false时丢弃该节点

    -   例如:

        ```ini
        #仅保留加密方式为chacha20的节点
        filter_script=function filter(node) {\n    if(node.EncryptMethod.includes('chacha20'))\n        return true;\n    return false;\n}
        # 或者使用本地文件
        filter_script="path:/path/to/script.js"
        ```

    -   node对象包含节点的全部信息，具体结构参见[此处](https://github.com/netchx/netch/blob/268bdb7730999daf9f27b4a81cfed5c36366d1ce/GSF.md)

11. **default_external_config**

    > 如果未指定外部配置文件，则将其设置为默认值。支持 `本地文件` 和 `在线URL`

    -   例如:

        ```ini
        default_external_config=config/example_external_config.ini
        ```

12. **base_path**

    > 限制外部配置可以使用的本地配置文件基础路径。

    -   例如:

        ```ini
        base_path=base
        #外部配置只可以使用base文件夹下的本地配置文件基础
        ```

13. **clash_rule_base**

    > 生成的 Clash 配置文件模板。支持 `本地文件` 和 `在线URL`

    -   例如:

        ```ini
        clash_rule_base=base/GeneralClashConfig.yml # 加载本地文件作为模板
        # 或者
        clash_rule_base=https://github.com/ACL4SSR/ACL4SSR/raw/master/Clash/GeneralClashConfig.yml
        # 加载ACL4SSR的 Github 中相关文件作为模板
        ```

14. **surge_rule_base**

    > 生成的 Surge 配置文件模板，用法同上

15. **surfboard_rule_base**

    > 生成的 Surfboard 配置文件模板，用法同上

16. **mellow_rule_base**

    > 生成的 Mellow 配置文件模板，用法同上

17. **loon_rule_base**

    > 生成的 Loon 配置文件模板，用法同上

18. **sssub_rule_base**

    > 生成的 sssub 配置文件模板，用法同上

19. **proxy_config**

    > 更新 外部配置文件 时是否使用代理
    >
    > 填写 `NONE` 或者空白禁用，或者填写 `SYSTEM` 使用系统代理
    >
    > 支持HTTP 或 SOCKS 代理(http&#x3A;// https&#x3A;// socks4a:// socks5://)
    >
    > 支持CORS代理(cors：)，详细参见[cors-anywhere](https://github.com/Rob--W/cors-anywhere)、[cloudflare-cors-anywhere](https://github.com/Zibri/cloudflare-cors-anywhere)等

    -   例如:

        ```ini
        proxy_config=SYSTEM # 使用系统代理
        # 或者
        proxy_config=socks5://127.0.0.1:1080 # 使用本地的 1080 端口进行 SOCKS5 代理
        # 或者
        proxy_config=cors:https://cors-anywhere.herokuapp.com/ # 使用CORS代理
        ```

20. **proxy_ruleset**

    > 更新 规则 时是否使用代理，用法同上

21. **proxy_subscription**

    > 更新 原始订阅 时是否使用代理，用法同上

22. **append_proxy_type**

    > 节点名称是否需要加入属性，设置为 true 时在节点名称前加入 \[SS] \[SSR] \[VMess] 以作区别，
    >
    > 默认为 false

    -   例如（设置为 true时）：

    ```txt
    [SS] 香港中转
    [VMess] 美国 GIA
    ```

</details>
<details>
<summary><b>[userinfo] 部分</b></summary>

> 该部分主要涉及到的内容为 **从节点名中提取用户信息的规则**
>
> 相关设置项目建议保持默认或者在知晓作用的前提下进行修改

1.  **stream_rule**

    > 从节点名中提取流量信息及显示的规则
    >
    > 使用方式：从节点提取信息的正则表达式|显示信息的正则表达式

    -   例如:

        ```ini
        stream_rule=^剩余流量：(.*?)\|总流量：(.*)$|total=$2&left=$1
        stream_rule=^剩余流量：(.*?) (.*)$|total=$1&left=$2
        stream_rule=^Bandwidth: (.*?)/(.*)$|used=$1&total=$2
        stream_rule=^\[.*?\]剩余(.*?)@(?:.*)$|total=$1
        stream_rule=^.*?流量:(.*?) 剩:(?:.*)$|total=$1
        ```

2.  **time_rule**

    > 从节点名中提取时间信息的规则
    >
    > 使用方式：从节点提取信息的正则表达式|显示信息的正则表达式

    -   例如:

        ```ini
        time_rule=^过期时间：(\d+)-(\d+)-(\d+) (\d+):(\d+):(\d+)$|$1:$2:$3:$4:$5:$6
        time_rule=^到期时间(:|：)(\d+)-(\d+)-(\d+)$|$1:$2:$3:0:0:0
        time_rule=^Smart Access expire: (\d+)/(\d+)/(\d+)$|$1:$2:$3:0:0:0
        time_rule=^.*?流量:(?:.*?) 剩:(.*)$|left=$1d
        ```

</details>
<details>
<summary><b>[node_pref] 部分</b></summary>

> 该部分主要涉及到的内容为 **开启节点的 UDP 及 TCP Fast Open** 、**节点的重命名** 、**重命名节点后的排序**
>
> 相关设置项目建议保持默认或者在知晓作用的前提下进行修改

1.  **udp_flag**

    > 为节点打开 UDP 模式，设置为 true 时打开，默认为 false

    -   当不清楚机场的设置时**请勿调整此项**。

2.  **tcp_fast_open_flag**

    > 为节点打开 TFO (TCP Fast Open) 模式，设置为 true 时打开，默认为 false

    -   当不清楚机场的设置时**请勿调整此项**。

3.  **skip_cert_verify_flag**

    > 关闭 TLS 节点的证书检查，设置为 true 时打开，默认为 false

    -   **请勿随意将此设置修改为 true**

4.  **tls13_flag**

    > 为节点增加tls1.3开启参数，设置为 true 时打开，默认为 false

    -   **请勿随意将此设置修改为 true**

5.  **sort_flag**

    > 对生成的订阅中的节点按节点名进行 A-Z 的排序，设置为 true 时打开，默认为 false

6.  **sort_script**

    > 对生成的订阅中的节点按自定义js函数进行排序
    >
    > 可设置为js代码内容，也可为本地js文件的路径
    >
    > js函数包括2个参数，即2个节点，函数返回为true时，节点a排在节点b的前方
    >
    > 具体细节参照 `[common]` 部分**filter_script**中的介绍

    -   例如:

        ```ini
        sort_script=function compare(node_a, node_b) {\n    return node_a.Remark > node_b.Remark;\n}
        # 或者
        sort_script="path:/path/to/script.js"
        ```

7.  **filter_deprecated_nodes**

    > 排除当前 **`target=`** 不支持的节点类型，设置为 true 时打开，默认为 false

    -   可以考虑设置为 true，从而在**一定程度上避免出现兼容问题**

8.  **append_sub_userinfo**

    > 在 header 里的加入流量信息 (Quanx, Surge 等读取后可以显示流量信息通知)，设置为 true 时打开，默认为 true

9.  **clash_use_new_field_name**

    > 启用 Clash 的新区块名称 (proxies, proxy-groups, rules)，设置为 true 时打开，默认为 true

    -   Clash内核在v0.19.0版本时开始启用新区块名称，当前已广泛使用v0.19.0及以上的版本，除非您确定正在使用极为古老的版本，否则请勿关闭。

10. **clash_proxies_style**

    > 在Clash配置文件中proxies的生成风格
    >
    > 可选的值为`block`、 `flow`、 `compact`，默认为`flow`

    -   风格示例：

        ```yaml
        Block:
          - name: name1
            key: value
          - name: name2
            key: value
        Flow:
          - {name: name1, key: value}
          - {name: name2, key: value}
        Compact:
         [{name: name1, key: value},{name: name2, key: value}]
        ```

11. **rename_node**

    > 重命名节点，支持正则匹配
    >
    > 使用方式：原始命名@重命名
    >
    > 可以使用自定义的js函数进行重命名
    >
    > 具体细节参照 `[common]` 部分**filter_script**中的介绍

    -   例如:

        ```ini
        rename_node=中国@中
        rename_node=\(?((x|X)?(\d+)(\.?\d+)?)((\s?倍率?:?)|(x|X))\)?@(倍率:$1)
        rename_node=!!script:function rename(node) {\n  const geoinfo = JSON.parse(geoip(node.Hostname));\n  if(geoinfo.country_code == "CN")\n    return "CN " + node.Remark;\n}
        rename_node=!!script:path:/path/to/script.js
        ```

    -   特殊用法:

        ```ini
        rename_node=!!GROUPID=0!!中国@中
        # 指定此重命名仅在第一个订阅的节点中生效
        ```

</details>
<details>
<summary><b>[managed_config] 部分</b></summary>

> 该部分主要涉及到的内容为 **订阅文件的更新地址**

1.  **write_managed_config**

    > 是否将 '#!MANAGED-CONFIG' 信息附加到 Surge 或 Surfboard 配置，设置为 true 时打开，默认为 true

2.  **managed_config_prefix**

    > 具体的 '#!MANAGED-CONFIG' 信息，地址前缀不用添加 "/"。
    >
    > Surge 或 Surfboard 会向此地址发出更新请求，同时本地 ruleset 转 url 会用此生成/getruleset链接。
    >
    > 局域网用户需要将此处改为本程序运行设备的局域网 IP

    -   例如:

    ```ini
    managed_config_prefix = http://192.168.1.5:25500
    ```

3.  **config_update_interval**

    > 托管配置更新间隔，确定配置将更新多长时间，单位为秒

    -   例如:

    ```ini
    config_update_interval = 86400
    # 每 86400 秒更新一次（即一天）
    ```

4.  **config_update_strict**

    > 如果 config_update_strict 为 true，则 Surge 将在上述间隔后要求强制更新。

5.  **quanx_device_id**

    > 用于重写 Quantumult X 远程 JS 中的设备 ID，该 ID 在 Quantumult X 设置中自行查找

    -   例如:

        ```ini
        quanx_device_id = XXXXXXX
        ```

</details>
<details>
<summary><b>[surge_external_proxy] 部分</b></summary>

> 为 Surge 添加 SSR 的支持路径

</details>
<details>
<summary><b>[emojis] 部分</b></summary>

1.  **add_emoji**

    > 是否在节点名称前加入下面自定义的 Emoji，设置为 true 时打开，默认为 true

2.  **remove_old_emoji**

    > 是否移除原有订阅中存在的 Emoji，设置为 true 时打开，默认为 true

3.  **rule**

    > 在匹配到的节点前添加自定义 emojis，支持正则匹配

    -   例如:

        ```ini
        rule=(流量|时间|应急),⌛time
        rule=(美|美国|United States),🇺🇸
        ```

    -   特殊用法:

        ```ini
        rule=!!GROUPID=0!!(流量|时间|应急),⌛time
        # 指定此 Emoji 规则仅在第一个订阅的节点中生效
        ```

</details>
<details>
<summary><b>[ruleset] 部分</b></summary>

> 如果你对原本订阅自带的规则不满意时，可以使用如下配置

1.  **enabled**

    > 启用自定义规则集的**总开关**，设置为 true 时打开，默认为 true

2.  **overwrite_original_rules**

    > 覆盖原有规则，即 `[common]` 中 xxx_rule_base 中的内容，设置为 true 时打开，默认为 false

3.  **update_ruleset_on_request**

    > 根据请求执行规则集更新，设置为 true 时打开，默认为 false

4.  **ruleset**

    > 从本地或 url 获取规则片段
    >
    > 格式为 `Group name,[type:]URL[,interval]` 或 `Group name,[]Rule `
    >
    > 支持的type（类型）包括：surge, quanx, clash-domain, clash-ipcidr, clash-classic
    >
    > type留空时默认为surge类型的规则
    >
    > \[] 前缀后的文字将被当作规则，而不是链接或路径，主要包含 `[]GEOIP` 和 `[]MATCH`(等同于 `[]FINAL`)。

    -   例如：

    ```ini
    ruleset=🍎 苹果服务,https://raw.githubusercontent.com/ACL4SSR/ACL4SSR/master/Clash/Apple.list
    # 表示引用 https://raw.githubusercontent.com/ACL4SSR/ACL4SSR/master/Clash/Apple.list 规则
    # 且将此规则指向 [proxy_group] 所设置 🍎 苹果服务 策略组
    ruleset=Domestic Services,clash-domain:https://ruleset.dev/clash_domestic_services_domains,86400
    # 表示引用clash-domain类型的 https://ruleset.dev/clash_domestic_services_domains 规则
    # 规则更新间隔为86400秒
    # 且将此规则指向 [proxy_group] 所设置 Domestic Services 策略组
    ruleset=🎯 全球直连,rules/NobyDa/Surge/Download.list
    # 表示引用本地 rules/NobyDa/Surge/Download.list 规则
    # 且将此规则指向 [proxy_group] 所设置 🎯 全球直连 策略组
    ruleset=🎯 全球直连,[]GEOIP,CN
    # 表示引用 GEOIP 中关于中国的所有 IP
    # 且将此规则指向 [proxy_group] 所设置 🎯 全球直连 策略组
    ruleset=!!import:snippets/rulesets.txt
    # 表示引用本地的snippets/rulesets.txt规则
    ```

</details>

<details>
<summary><b>[proxy_group] 部分</b></summary>

> 为 Clash 、Mellow 、Surge 以及 Surfboard 等程序创建策略组, 可用正则来筛选节点
>
> \[] 前缀后的文字将被当作引用策略组

```ini
custom_proxy_group=Group_Name`url-test|fallback|load-balance`Rule_1`Rule_2`...`test_url`interval[,timeout][,tolerance]
custom_proxy_group=Group_Name`select`Rule_1`Rule_2`...
# 格式示例
custom_proxy_group=🍎 苹果服务`url-test`(美国|US)`http://www.gstatic.com/generate_204`300,5,100
# 表示创建一个叫 🍎 苹果服务 的 url-test 策略组,并向其中添加名字含'美国','US'的节点，每隔300秒测试一次，测速超时为5s，切换节点的延迟容差为100ms
custom_proxy_group=🇯🇵 日本延迟最低`url-test`(日|JP)`http://www.gstatic.com/generate_204`300,5
# 表示创建一个叫 🇯🇵 日本延迟最低 的 url-test 策略组,并向其中添加名字含'日','JP'的节点，每隔300秒测试一次，测速超时为5s
custom_proxy_group=负载均衡`load-balance`.*`http://www.gstatic.com/generate_204`300,,100
# 表示创建一个叫 负载均衡 的 load-balance 策略组,并向其中添加所有的节点，每隔300秒测试一次，切换节点的延迟容差为100ms
custom_proxy_group=🇯🇵 JP`select`沪日`日本`[]🇯🇵 日本延迟最低
# 表示创建一个叫 🇯🇵 JP 的 select 策略组,并向其中**依次**添加名字含'沪日','日本'的节点，以及引用上述所创建的 🇯🇵 日本延迟最低 策略组
custom_proxy_group=节点选择`select`(^(?!.*(美国|日本)).*)
# 表示创建一个叫 节点选择 的 select 策略组,并向其中**依次**添加名字不包含'美国'或'日本'的节点
```

-   还可使用一些特殊筛选条件：

    `` `!!GROUPID=%n%`` 待转换链接中的第 n+1 条链接中包含的节点

    `` `!!INSERT=%n%`` 配置文件中 `insert_url` 的第 n+1 条链接所包含的节点

    `` `!!PROVIDER=%proxy-provider-name%`` 指定名称的proxy-provider

    GROUPID 和 INSERT 匹配支持range,如 1,!2,3-4,!5-6,7+,8-

    ```ini
    custom_proxy_group=g1`select`!!GROUPID=0`!!INSERT=0
    # 表示创建一个叫 g1 的 select 策略组,并向其中依次添加订阅链接中第一条订阅链接中的所有节点和配置文件中 insert_url 中的**第一个**节点
    custom_proxy_group=g2`select`!!GROUPID=1
    # 表示创建一个叫 g2 的 select 策略组,并向其中依次添加订阅链接中第二条订阅链接中的所有节点
    custom_proxy_group=g3`select`!!GROUPID=!2
    # 表示创建一个叫 g3 的 select 策略组,并向其中依次添加订阅链接中除了第三条订阅链接之外的所有节点
    custom_proxy_group=g4`select`!!GROUPID=3-5
    # 表示创建一个叫 g4 的 select 策略组,并向其中依次添加订阅链接中第四条到第六条订阅链接中的所有节点
    custom_proxy_group=v2ray`select`!!GROUP=V2RayProvider
    # 表示创建一个叫 v2ray 的 select 策略组,并向其中依次添加订阅链接中组名（tag）为 V2RayProvider 的所有节点
    ```

    注意：此处的订阅链接指 `default_url` 和 `&url=` 中的订阅以及单链接节点（区别于配置文件中 insert_url）

-   现在也可以使用2个条件组合来进行筛选，只有同时满足这2个筛选条件的节点才会被加入组内

    ```ini
    custom_proxy_group=g1hk`select`!!GROUPID=0!!(HGC|HKBN|PCCW|HKT|hk|港)
    # 属于订阅链接中的第一条订阅**且**名字含 HGC、HKBN、PCCW、HKT、hk、港 的节点
    ```

-   也可以使用js脚本筛选加入策略组的节点。A "filter" function with one argument which is an array of all available nodes should be defined in the script.

    ```ini
    custom_proxy_group=script`select`script:/path/to/script.js
    # 表示创建一个叫 script 的 select 策略组，其中的节点使用本地的/path/to/script.js脚本中的函数进行筛选
    ```

-   也可以使用本地文件

    ```ini
    custom_proxy_group=!!import:snippets/groups.txt
    # 使用本地的snippets/groups.txt文件
    ```

</details>

<details>
<summary><b>[aliases] 部分</b></summary>

> 设置访问接口的别名，也可以用来缩短URI。
>
> 访问别名时会将传递的所有参数附加到别名目标的参数中。

使用方法如下（但不仅限于此）：

-   精简接口步骤（此类别名默认在 pref 中启用）

    ```ini
    当设置 /clash=/sub?target=clash 时：
    访问 127.0.0.1/clash?url=xxx 即跳转至 127.0.0.1/sub?target=clash&url=xxx
    ```

-   精简外部配置路径

    ```ini
    当设置 /mysub=/getprofile?name=aaa&token=bbb 时：
    访问 127.0.0.1/mysub 即跳转至 127.0.0.1/getprofile?name=aaa&token=bbb
    ```

</details>

<details>
<summary><b>[tasks] 部分</b></summary>

> 该部分主要涉及到的内容为 **定时执行js文件中的代码**

1.  **task**

    > 在服务器运行期间定期执行的任务。
    >
    > 使用方式 任务名称\`Cron表达式\`JS文件路径\`超时时间(s)

    -   例如:

        ```ini
        task=tick`0/10 * * * * ?`tick.js`3
        ```

</details>

<details>
<summary><b>[server] 部分</b></summary>

> 此部分通常**保持默认**即可

1.  **listen**

    > 绑定到 Web 服务器的地址，将地址设为 0.0.0.0，则局域网内设备均可使用

2.  **port**

    > 绑定到 Web 服务器地址的端口，默认为 25500

3.  **serve_file_root**

    > Web服务器的根目录，可以为包含静态页面的文件夹，留空则为关闭

</details>

<details>
<summary><b>[template] 部分</b></summary>

> 此部分用于指定 模板 中的部分值

1.  **template_path**

    > 对**子模板**文件的所在位置(即模板文件中使用 `{% include "xxx.tpl" %}` 引入的模板)做出路径限制

2.  **clash.dns 等**

    > 名称可以为任意非本程序默认的参数，用来对模板中的值进行判断或在模板中使用其定义的参数

</details>

<details>

<summary><b>[advanced] 部分</b></summary>

> 此部分通常**保持默认**即可

1.  **log_level**

    > 日志级别，可选值有：fatal error warn info debug verbose

2.  **print_debug_info**

    > 是否打印debug信息

3.  **max_pending_connections**

    > 最大挂起连接数

4.  **max_concurrent_threads**

    > 最大线程数

5.  **max_allowed_rulesets**

    > 规则集数量上限，0表示无限

6.  **max_allowed_rules**

    > 规则数量上限，0表示无限

7.  **max_allowed_download_size**

    > subconverter下载外部文件时的文件大小上限，超过时直接忽略该文件，单位bytes，0表示无限

8.  **enable_cache**

    > 是否启用缓存

9.  **cache_subscription**

    > 当启用缓存时，订阅文件的缓存时间

10. **cache_config**

    > 当启用缓存时，外部配置文件的缓存时间

11. **cache_ruleset**

    > 当启用缓存时，规则集的缓存时间

12. **script_clean_context**

    > script脚本是否使用干净上下文

13. **async_fetch_ruleset**

    > 并行下载规则集

14. **skip_failed_links**

    > 跳过失败的链接，继续转换而不是直接返回错误

</details>

### 外部配置

> 本部分用于 链接参数 **`&config=`**

注：本部分内容以本程序中的 [`/config/example_external_config.ini`](https://github.com/tindy2013/subconverter/blob/master/base/config/example_external_config.ini) 或 [`/config/example_external_config.yml`](https://github.com/tindy2013/subconverter/blob/master/base/config/example_external_config.yml) 或 [`/config/example_external_config.toml`](https://github.com/tindy2013/subconverter/blob/master/base/config/example_external_config.toml) 为准，本文档可能由于更新不及时，内容不适用于新版本。

将文件按照以下格式写好，上传至 Github Gist 或者 其他**可访问**网络位置
经过 [URLEncode](https://www.urlencoder.org/) 处理后，添加至 `&config=` 即可调用
需要注意的是，由外部配置中所定义的值会**覆盖** 主程序目录中配置文件 里的内容

即，如果你在外部配置中定义了

```txt
emoji=(流量|时间|应急),🏳️‍🌈
emoji=阿根廷,🇦🇷
```

那么本程序只会匹配以上两个 Emoji，不再使用 主程序目录中配置文件 中所定义的 国别 Emoji

<details>
<summary><b>点击查看文件内容</b></summary>

```ini
[custom]
;这是一个外部配置文件示例
;所有可能的自定义设置如下所示

;用于自定义组的选项 会覆盖 主程序目录中的配置文件 里的内容
;使用以下模式生成 Clash 代理组，带有 "[]" 前缀将直接添加
;Format: Group_Name`select`Rule_1`Rule_2`...
;        Group_Name`url-test|fallback|load-balance`Rule_1`Rule_2`...`test_url`interval[,timeout][,tolerance]
;Rule with "[]" prefix will be added directly.

custom_proxy_group=Proxy`select`.*`[]AUTO`[]DIRECT`.*
custom_proxy_group=UrlTest`url-test`.*`http://www.gstatic.com/generate_204`300,5,100
custom_proxy_group=FallBack`fallback`.*`http://www.gstatic.com/generate_204`300,5
custom_proxy_group=LoadBalance`load-balance`.*`http://www.gstatic.com/generate_204`300,,100
custom_proxy_group=SSID`ssid`default_group`celluar=group0,ssid1=group1,ssid2=group2

;custom_proxy_group=g1`select`!!GROUPID=0
;custom_proxy_group=g2`select`!!GROUPID=1
;custom_proxy_group=v2ray`select`!!GROUP=V2RayProvider

;custom_proxy_group=g1hk`select`!!GROUPID=0!!(HGC|HKBN|PCCW|HKT|hk|港)
;custom_proxy_group=sstw`select`!!GROUP=V2RayProvider!!(深台|彰化|新北|台|tw)
;custom_proxy_group=provider`select`!!PROVIDER=prov1,prov2,prov3`fallback_nodes


;用于自定义规则的选项 会覆盖 主程序目录中的配置文件 里的内容
;Ruleset addresses, supports local files/URL
;Format: Group name,[type:]URL[,interval]
;        Group name,[]Rule
;where "type" supports the following value: surge, quanx, clash-domain, clash-ipcidr, clash-classic
;type defaults to surge if omitted
enable_rule_generator=false
overwrite_original_rules=false
;ruleset=DIRECT,https://raw.githubusercontent.com/DivineEngine/Profiles/master/Surge/Ruleset/Guard/Unbreak.list,86400
;ruleset=🎯 全球直连,rules/LocalAreaNetwork.list
;ruleset=DIRECT,surge:rules/LocalAreaNetwork.list
;ruleset=Advertising,quanx:https://raw.githubusercontent.com/DivineEngine/Profiles/master/Quantumult/Filter/Guard/Advertising.list,86400
;ruleset=Domestic Services,clash-domain:https://ruleset.dev/clash_domestic_services_domains,86400
;ruleset=Domestic Services,clash-ipcidr:https://ruleset.dev/clash_domestic_services_ips,86400
;ruleset=DIRECT,clash-classic:https://raw.githubusercontent.com/DivineEngine/Profiles/master/Clash/RuleSet/China.yaml,86400
;ruleset=🎯 全球直连,[]GEOIP,CN
;ruleset=🐟 漏网之鱼,[]FINAL

;用于自定义基础配置的选项 会覆盖 主程序目录中的配置文件 里的内容
clash_rule_base=base/forcerule.yml
;surge_rule_base=base/surge.conf
;surfboard_rule_base=base/surfboard.conf
;mellow_rule_base=base/mellow.conf
;quan_rule_base=base/quan.conf
;quanx_rule_base=base/quanx.conf

;用于自定义重命名的选项 会覆盖 主程序目录中的配置文件 里的内容
;rename=Test-(.*?)-(.*?)-(.*?)\((.*?)\)@\1\4x测试线路_自\2到\3
;rename=\(?((x|X)?(\d+)(\.?\d+)?)((\s?倍率?)|(x|X))\)?@$1x

;用于自定义 Emoji 的选项 会覆盖 主程序目录中的配置文件 里的内容
;add_emoji=true
;remove_old_emoji=true
;emoji=(流量|时间|应急),🏳️‍🌈
;emoji=阿根廷,🇦🇷

;用于包含或排除节点关键词的选项 会覆盖 主程序目录中的配置文件 里的内容
;include_remarks=
;exclude_remarks=

;[template]
;;局部作用于模板中的变量
;clash.dns.port=5353
```

</details>

### 模板功能

> `0.5.0` 版本中引进了模板功能，可以通过设置不同的条件参数来获取对应的模板内容
>
> 从而做到将多个模板文件合成为一个，或者在不改动模板内容的前提下修改其中的某个参数等

#### 模板调用

当前模板调用可以用于 [外部配置](#外部配置) 和各类 base 文件中，示例可以参照 [all_base.tpl](./base/base/all_base.tpl)

模板内的常用写法有以下几类：

> 各种判断可以嵌套使用，但需要确保逻辑关系没有问题，即有 `if` 就要有 `endif`
>
> 更多的使用方式可以参照 [INJA 语法](https://github.com/pantor/inja)

1.  取值

    ```inja
    {{ global.clash.http_port }}
    # 获取 配置文件 中 clash.http_port 的值
    ```

2.  单判断

    ```inja
    {% if request.clash.dns == "1" %}
    ···
    {% endif %}
    # 如果 URL 中的 clash.dns=1 时，判断成立
    ```

3.  或判断

    ```inja
    {% if request.target == "clash" or request.target == "clashr" %}
    ···
    {% endif %}
    # 如果 URL 中的 target 为 clash 或者 clashr 时，判断成立
    ```

4.  如果...否则...

    ```inja
    {% if local.clash.new_field_name == "true" %}
    proxies: ~
    proxy-groups: ~
    rules: ~
    {% else %}
    Proxy: ~
    Proxy Group: ~
    Rule: ~
    {% endif %}
    # 如果 外部配置中 clash.new_field_name=true 时，启用 新的 Clash 块名称，否则使用旧的名称
    ```

5.  如果存在...则...(可避免请求中无对应参数时发生的报错)

    ```inja
    {% if exists("request.clash.dns") %}
    dns:
      enabled: true
      listen: 1053
    {% endif %}
    # 如果 URL 中存在对 clash.dns 参数的任意指定时，判断成立 (可以和 如果···否则··· 等判断一起使用)
    ```

6.  单判断，且如果参数不存在时使用默认值进行判断(可避免请求中无对应参数时发生的报错)

    ```inja
    dns:
      enabled: true
      listen: 1053
      nameserver:
       {% if default(request.doh, "false") == "true" %}
       - https://doh.pub/dns-query
       - https://223.5.5.5/dns-query
       {% else %}
       - 119.29.29.29
       - 223.5.5.5
       {% endif %}
    # 如果 URL 中 doh 参数为 true 时，判断成立。
    # 如果 URL 中不存在 doh 参数时，将 clash.doh 参数设为默认值 false 再进行判断。
    ```

模板内的引用有以下几类：

1.  从 配置文件 中获取，判断前缀为 `global`

    ```inja
    socks-port: {{ global.clash.socks_port }}
    # 当配置文件中设定了 `clash.socks_port` 值时，将被引用
    ```

2.  从 外部配置 中获取，判断前缀为 `local`

    ```inja
    {% if local.clash.new_field_name =="true" %}
    ···
    {% endif %}
    # 当外部配置中设定了 `clash.new_field_name=true` 时，该判断生效，其包含的···内容被引用
    ```

3.  从 URL 链接中获取，判断前缀为 `request`，例如 `http://127.0.0.1:25500/sub?target=clash&url=www.xxx.com&clash.dns=1`

    -   从 URL 中所获得**包含**在 [进阶链接](#进阶链接) 内的参数进行判断

        ```inja
        {% if request.target == "clash" %}
        ···
        {% endif %}
        # 当 target=clash 时，该判断生效，其包含的··· 内容被引用
        ```

    -   从 URL 中所获得**不包含**在 [进阶链接](#进阶链接) 内的参数进行判断 (从上述链接可以看出 clash.dns 属于额外参数)

        ```inja
        {% if request.clash.dns == "1" %}
        dns:
          enabled: true
          listen: 1053
        {% endif %}
        # 当 clash.dns=1 时，该判断生效，其包含的 dns 内容被引用
        ```

#### 直接渲染

在对模板功能进行调试或需要直接对模板进行渲染时，此时可以使用以下方式进行调用

```txt
http://127.0.0.1:25500/render?path=xxx&额外的调试或控制参数
```

此处 `path` 需要在 [配置文件](#配置文件) 中 `template_path` 所限定的路径内

## 特别用法

### 本地生成

> 启动程序后，在本地生成对应的配置文件文本

在程序目录内的 [generate.ini](https://github.com/tindy2013/subconverter/blob/master/base/generate.ini) 中设定文件块(`[xxx]`)，生成的文件名(path=xxx)以及其所需要包含的参数，例如：

```ini
[test]
path=output.conf
target=surge
ver=4
url=ss://Y2hhY2hhMjAtaWV0Zi1wb2x5MTMwNTpwYXNzd29yZA@www.example.com:1080#Example

[test_profile]
path=output.yml
profile=profiles/example_profile.ini
```

使用 `subconverter -g` 启动本程序时，即可在程序根目录内生成名为 `output.conf` `output.yml` 的配置文件文本。

使用 `subconverter -g --artifact "test"` 启动本程序时，即可在程序根目录内仅生成上述示例中 `[test]` 文件块所指代的 `output.conf` 的配置文件文本。

### 自动上传

> 自动上传 gist ，可以用于 Clash For Android / Surge 等进行远程订阅

在程序目录内的 [gistconf.ini](https://github.com/tindy2013/subconverter/blob/master/base/gistconf.ini) 中添加 `Personal Access Token`（[在此创建](https://github.com/settings/tokens/new?scopes=gist&description=Subconverter)）例如：

```ini
[common]
;uncomment the following line and enter your token to enable upload function
token = xxxxxxxxxxxxxxxxxxxxxxxx(所生成的 Personal Access Token)
```

在 [调用地址](#调用地址) 或 [调用地址 (进阶)](#调用地址-进阶) 所生成的链接后加上 `&upload=true` 就会在更新好后自动上传 gist
此时，subconverter 程序窗口内会出现如下所示的**提示信息**：

```cmd
No gist id is provided. Creating new gist...
Writing to Gist success!
Generator: surge4
Path: surge4
Raw URL: https://gist.githubusercontent.com/xxxx/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/raw/surge4
Gist owner: xxxx
```

上方所提到的 `Raw URL: https://gist.githubusercontent.com/xxxx/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/raw/surge4`
中的 `https://gist.githubusercontent.com/xxxx/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx/raw/surge4` 即是你的在线订阅链接。

注意，本程序默认会将此链接设为**秘密状态**

根据 [`官方手册 - 创建 Gist`](https://help.github.com/cn/github/writing-on-github/creating-gists) 的解释为：

> 秘密 gists 不会显示在 Discover 中，也不可搜索。
>
> 秘密 gists 不是私人的。 如果将秘密 gist 的 URL 发送给朋友，他们可以查看。
>
> 但是，如果您不认识的人发现该 URL，也能看到您的 gist。

所以请务必保管好所生成的 `Raw URL` 链接。

### 规则转换

> 将规则转换为指定的规则类型，用于将不同类型的规则互相转换

#### 调用地址 (规则转换)

```txt
http://127.0.0.1:25500/getruleset?type=%TYPE%&url=%URL%&group=%GROUP%
```

#### 调用说明 (规则转换)

| 调用参数  |    必要性    | 示例      | 解释                                                                                                                                                       |
| ----- | :-------: | :------ | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| type  |     必要    | 6       | 指想要生成的规则类型，用数字表示：1为Surge，2 为 Quantumult X，3 为 Clash domain rule-provider，4 为 Clash ipcidr rule-provider，5 为 Surge DOMAIN-SET，6 为 Clash classical ruleset |
| url   |     必要    |         | 指待转换的规则链接，需要经过 [Base64](https://base64.us/) 处理                                                                                                           |
| group | type=2时必选 | mygroup | 规则对应的策略组名，生成Quantumult X类型（type=2）时必须提供                                                                                                                  |

运行 subconverter 主程序后， 按照 [调用地址 (规则转换)](#调用地址-规则转换) 的对应内容替换即可得到指定类型的规则。

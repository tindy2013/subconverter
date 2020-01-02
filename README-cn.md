# subconverter

在各种订阅格式之间进行转换的实用程序.

[![Build Status](https://travis-ci.com/tindy2013/subconverter.svg?branch=master)](https://travis-ci.com/tindy2013/subconverter)

- [支持类型](#支持类型)
- [简易用法](#简易用法)
  - [调用地址](#调用地址)
  - [调用说明](#调用说明)
- [进阶用法](#进阶用法)
  - [阅读提示](#阅读提示)
  - [进阶地址](#进阶地址)
  - [pref.ini 的说明](#pref.ini的说明)
- [自动上传](#自动上传)

## 支持类型

| 类型         | 作为源类型 | 作为目标类型 | 参数        |
| ------------ | :--------: | :----------: | ----------- |
| Clash        |     ✔      |      ✔       | clash       |
| ClashR       |     ✔      |      ✔       | clashr      |
| Quantumult   |     ✔      |      ✔       | quan        |
| Quantumult X |     ✔      |      ✔       | quanx       |
| SS (SIP002)  |     ✔      |      ✔       | ss          |
| SSD          |     ✔      |      ✔       | ssd         |
| SSR          |     ✔      |      ✔       | ssr         |
| Surfboard    |     ✔      |      ✔       | surfboard   |
| Surge 2      |     ✔      |      ✔       | surge&ver=2 |
| Surge 3      |     ✔      |      ✔       | surge&ver=3 |
| Surge 4      |     ✔      |      ✔       | surge&ver=4 |
| V2Ray        |     ✔      |      ✔       | v2ray       |

**注意**：Shadowrocket 用户可以使用 `ss`、`ssr`以及 `v2ray`参数

---

## 简易用法

> 即生成的配置文件默认套用 **神机规则**

### 调用地址

```TXT
http://127.0.0.1:25500/sub?target=%TARGET%&url=%URL%&config=%CONFIG%
```

### 调用说明

| 调用参数 | 必要性 | 示例      | 解释         |
| ------- | :----: | :------------------- | ---------------- |
| target |  必要  | surge&ver=4   | 指想要生成的配置类型，详见上方 [支持类型](#支持类型) 中的参数 |
| url  |  必要  | https%3A%2F%2Fwww.xxx.com | 指机场所提供的订阅链接，需要经过 [URLEncode](https://www.urlencoder.org/) 处理 |
| config |  可选  | https%3A%2F%2Fwww.xxx.com | 指远程 `pref.ini` (包含分组和规则部分)，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，可查看 [示例仓库](https://github.com/lzdnico/subconverteriniexample) 寻找灵感，默认加载本地设置文件 |

运行 `subconverter.exe` 后，按照 [调用说明](###调用说明) 的对应内容替换即可得到一份使用**神机规则**的配置文件。

此外，如果你需要将多个订阅合成一份, 则要在上方所提及的 URLEncode 之前使用 '|' 来分隔链接。

举个例子：

```TXT
有以下两个订阅，且想合并转换成 Clash 的订阅:
1. https://dler.cloud/subscribe/ABCDE?clash=vmess
2. https://rich.cloud/subscribe/ABCDE?clash=vmess

首先使用 '|' 将两个订阅分隔开:
https://dler.cloud/subscribe/ABCDE?clash=vmess|https://rich.cloud/subscribe/ABCDE?clash=vmess

接着通过 URLEncode 后可以得到:
https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess%7Chttps%3A%2F%2Frich.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

然后将想要的 %TARGET% (即 clash)和上一步所得到的 %URL% 填入调用地址中:
http://127.0.0.1:25500/sub?target=clash&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess%7Chttps%3A%2F%2Frich.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

最后将该链接填写至 Clash 的订阅处就大功告成了。
```

---

## 进阶用法

> 在不满足于本程序所提供的神机规则或者对应的分组时，可以考虑尝试进阶用法
> 即 对 `调用地址` 甚至程序目录下的 `pref.ini` 进行个性化的编辑以满足不同的需求

### 阅前提示

在进行下一步操作前，十分推荐您阅读以下内容：

1. 与 `pref.ini` 相关的：[INI 语法介绍](https://zh.wikipedia.org/wiki/INI%E6%96%87%E4%BB%B6)
1. 与 `Clash` 配置相关的： [YAML 语法介绍](https://zh.wikipedia.org/wiki/YAML#%E8%AA%9E%E6%B3%95)
1. 会经常涉及到的： [正则表达式入门](https://github.com/ziishaned/learn-regex/blob/master/translations/README-cn.md)
1. 当遇到问题需要提交 ISSUE 时的： [提问的智慧](https://github.com/ryanhanwu/How-To-Ask-Questions-The-Smart-Way/blob/master/README-zh_CN.md)

当您尝试进行进阶操作时，即默认您有相关的操作能力，本程序仅能保证程序能够正常运行。

### 进阶地址

#### 调用地址 (进阶)

```txt
http://127.0.0.1:25500/sub?target=%TARGET%&url=%URL%&emoji=%EMOJI%····
```

#### 调用说明 (进阶)

| 调用参数 | 必要性 | 示例  | 解释   |
| -------- | :----: | :--------------- | :------------------------ |
| target |  必要  | surge&ver=4   | 指想要生成的配置类型，详见上方 [支持类型](#支持类型) 中的参数   |
| url   |  可选  | https%3A%2F%2Fwww.xxx.com | 指机场所提供的订阅链接，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，**可选的前提是在 `default_url` 中进行指定**    |
| config |  可选  | https%3A%2F%2Fwww.xxx.com | 指远程 `pref.ini` (包含分组和规则部分)，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，可查看 [示例仓库](https://github.com/lzdnico/subconverteriniexample) 寻找灵感，默认加载本地设置文件 |
| upload |  可选  | true / false  | 指将生成的订阅文件上传至 `Gist`，需要填写`gistconf.ini`，默认为 false (即不上传)    |
| emoji |  可选  | true / false  | 指在节点名称前加入 Emoji，默认为 true  |
| group |  可选  | MySS  | 指设置该订阅的组名，多用于 SS/SSD/SSR/V2Ray  |
| tfo |  可选  | true / false  | 指开启该订阅链接的 TCP Fast Open，默认为 false  |
| udp |  可选  | true / false  | 指开启该订阅链接的 UDP，默认为 false  |
| scv |  可选  | true / false  | 指跳过原始订阅的证书验证，默认为 true  |
| list |  可选  | true / false  | 指输出 Surge nodelist 或者 Clash proxy provider  |
| sort |  可选  | true / false  | 指对输出的节点或策略组进行再次排序，默认为 false  |
| include |  可选  | 详见下文中 `include_remarks`  | 指仅保留匹配到的节点，支持正则匹配，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，会覆盖配置文件里的设置  |
| exclude |  可选  | 详见下文中 `exclude_remarks`  | 指排除匹配到的节点，支持正则匹配，需要经过 [URLEncode](https://www.urlencoder.org/) 处理，会覆盖配置文件里的设置  |

举个例子：

```TXT
有订阅 `https://dler.cloud/subscribe/ABCDE?clash=vmess`，想转换成 Surge 4 的订阅，且需要开启 TFO 和 UDP
顺便再给节点名加上 EMOJI 同时排除掉订阅中显示流量和官网的节点（节点名为"剩余流量：1024G"，"官网地址：dler.cloud"）

首先确认需要用到的参数：
surge&ver=4 、 tfo=true 、 udp=true 、 emoji=true 、exclude=(流量|官网)
url=https://dler.cloud/subscribe/ABCDE?clash=vmess

然后将需要 URLEncode 的部分进行处理：
exclude=%28%E6%B5%81%E9%87%8F%7C%E5%AE%98%E7%BD%91%29
url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

接着将所有元素进行拼接：
http://127.0.0.1:25500/sub?surge&ver=4&tfo=true&udp=true&emoji=true&exclude=%28%E6%B5%81%E9%87%8F%7C%E5%AE%98%E7%BD%91%29&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess

最后将该链接填写至 Surge 的订阅处就大功告成了。
```

### pref.ini的说明

#### [common] 部分

> 该部分主要涉及到的内容为 **全局的节点排除或保留** 、**节点的重命名**
> 其他设置项目可以保持默认或者在知晓作用的前提下进行修改

1. **api_mode**

    > API 模式，设置为 true 以防止直接加载本地订阅或直接提供本地文件。（多用于架设于服务器上）

    - 当值为 `false` 时, 每次更新配置都会读取 `pref.ini` , 为 `true` 时则仅启动时读取。

1. **default_url**

    > 无 %URL% 参数时，默认加载的订阅链接， **不需要 URLEncode**。 如果有多个链接，仍然需要使用 "|" 分隔，支持`文件`/`url`

    - 例如:

     ```ini
     default_url='https://dler.cloud/subscribe/ABCDE?clash=vmess'
     ```

    - 解释：

     ```TXT
     此时订阅链接:
     http://127.0.0.1:25500/sub?target=clash
     等同于:
     http://127.0.0.1:25500/sub?target=clash&url=https%3A%2F%2Fdler.cloud%2Fsubscribe%2FABCDE%3Fclash%3Dvmess
     ```

1. **exclude_remarks**

   > 排除匹配到的节点，支持正则匹配

    - 例如:

     ```ini
     exclude_remarks=(流量|时间|官网|产品)
     ```

1. **include_remarks**

   > 仅保留匹配到的节点，支持正则匹配

    - 例如:

     ```ini
     include_remarks=(?<=美).*(BGP|GIA|IPLC)
     ```

1. **clash_rule_base**

   > 生成的 Clash 配置文件基础。支持 `本地文件` 和 `在线URL`

    - 例如:

     ```ini
     clash_rule_base=clash.yaml # 加载本地的 clash.yaml 文件作为基础
     # 或者
     clash_rule_base=https://raw.githubusercontent.com/ConnersHua/Profiles/master/Clash/Pro.yaml
     # 加载神机的 Github 中相关文件作为基础
     ```

1. **surge_rule_base**

   > 生成的 Surge 配置文件基础，用法同上

1. **surfboard_rule_base**

   > 生成的 Surfboard 配置文件基础，用法同上

1. **mellow_rule_base**

   > 生成的 Mellow 配置文件基础，用法同上

1. **proxy_ruleset**

   > 更新 RuleSet 时是否使用代理
   > 填写 `NONE` 或者空白禁用，或者填写 `SYSTEM` 使用系统代理
   > 也可填写如同 `socks5://127.0.0.1:1080` 的 HTTP 或 SOCKS 代理

    - 例如:

     ```ini
     proxy_ruleset=SYSTEM # 使用系统代理
     # 或者
     proxy_ruleset=socks5://127.0.0.1:1080 # 使用本地的 1080 端口进行 SOCKS5 代理
     ```

1. **proxy_subscription**

   > 更新 原始订阅 时是否使用代理，用法同上

1. **append_proxy_type**

   > 节点名称是否需要加入属性，设置为 true 时在节点名称前加入 \[SS\] \[SSR\] \[VMess\] 以作区别，
   > 默认为 false

    - 例如（设置为 true时）：

     ```txt
     [SS] 香港中转
     [VMess] 美国 GIA
     ```

1. **rename_node**

   > 重命名节点，支持正则匹配
   > 使用方式：原始命名@重命名

    - 例如:

     ```ini
     rename_node=中国@中
     rename_node=深圳@深
     ```

#### [node_pref] 部分

> 该部分主要涉及到的内容为 **开启节点的UDP及TCP** 、**重命名节点后的排序**
> 相关设置项目建议保持默认或者在知晓作用的前提下进行修改

1. **udp_flag**

   > 为节点打开 UDP 模式，设置为 true 时打开，默认为 false

    - 当不清楚机场的设置时**请勿调整此项**。

1. **tcp_fast_open_flag**

   > 为节点打开 TFO (TCP Fast Open) 模式，设置为 true 时打开，默认为 false。

    - 当不清楚机场的设置时**请勿调整此项**。

1. **sort_flag**

   > 对生成的订阅中的节点进行 A-Z 的排序，设置为 true 时打开，默认为 false。

1. **skip_cert_verify_flag**

   > 跳过原始订阅链接的证书检查，设置为 true 时打开，默认为 true

#### [managed_config] 部分

> 该部分主要涉及到的内容为 **订阅文件的更新地址**

1. **write_managed_config**

   > 是否将'＃!MANAGED-CONFIG'信息附加到 Surge 或 Surfboard 配置，设置为 true 时打开，默认为 true

1. **managed_config_prefix**

   > 具体的 '#!MANAGED-CONFIG' 信息，地址前缀不用添加 "/"
   > Surge 或 Surfboard 会向此地址发出更新请求
   > 局域网用户需要将此处改为对应的局域网 ip

    - 例如:

    ```ini
    managed_config_prefix = http://192.168.1.5:25500
    ```

#### [surge_external_proxy] 部分

> 为 Surge 添加 SSR 的支持路径

#### [emojis] 部分

1. add_emoji

   > 是否在节点名称前加入下面自定义的 Emoji，设置为 true 时打开，默认为 true

1. remove_old_emoji

   > 是否移除原有订阅中存在的 Emoji，设置为 true 时打开，默认为 true

1. rule

   > 在匹配到的节点前添加自定义 emojis，支持正则匹配

    - 例如:

    ```ini
    rule=(流量|时间|应急),⌛time
    rule=(美|美国|United States),🇺🇸
    ```

#### [ruleset] 部分

> 如果你对原本订阅自带的规则不满意可以使用如下配置

1. **enabled**

   > 启用自定义规则集，设置为 true 时打开，默认为 true

1. **overwrite_original_rules**

   > 覆盖原有规则，即 [common] 中 xxx_rule_base 中的内容
   > 设置为 true 时打开，默认为 false

1. **update_ruleset_on_request**

   > 根据请求执行规则集更新
   > 设置为 true 时打开，默认为 false

1. **surge_ruleset**

   > 从本地或 url 获取规则片段.
   > [] 前缀后的文字将被当作规则，而不是链接或路径

    - 例如：

    ```ini
    surge_ruleset=🍎 苹果服务,https://raw.githubusercontent.com/ConnersHua/Profiles/master/Surge/Ruleset/Apple.list
    # 表示引用 https://raw.githubusercontent.com/ConnersHua/Profiles/master/Surge/Ruleset/Apple.list 规则
    # 且将此规则指向 [clash_proxy_group] 所设置 🍎 苹果服务 策略组
    surge_ruleset=🎯 全球直连,rules/NobyDa/Surge/Download.list
    # 表示引用本地 rules/NobyDa/Surge/Download.list 规则
    # 且将此规则指向 [clash_proxy_group] 所设置 🎯 全球直连 策略组
    surge_ruleset=🎯 全球直连,[]GEOIP,CN
    # 表示引用 GEOIP 中关于中国的所有 IP
    # 且将此规则指向 [clash_proxy_group] 所设置 🎯 全球直连 策略组
    ```

#### [clash_proxy_group] 部分

> 为 Clash 、Mellow 、Surge 以及 Surfboard 等程序创建策略组, 可用正则来筛选节点

```ini
custom_proxy_group=🍎 苹果服务`url-test`(美国|US)`http://www.gstatic.com/generate_204`300
# 表示创建一个叫 🍎 苹果服务 的 url-test策略组,并向其中添加名字含'美国','US'的节点，每隔300秒测试一次
custom_proxy_group=🇯🇵 JP`select`沪日`日本
# 表示创建一个叫 🇯🇵 JP 的 select 策略组,并向其中**依次**添加名字含'沪日','日本'的节点
```

- ssr/v2 订阅默认没有组名, 可以使用这个方法来添加组名

  ```ini
  custom_proxy_group=g1`select`!!GROUPID=0
  # 指订阅链接中的第一条订阅
  custom_proxy_group=g2`select`!!GROUPID=1
  # 指订阅链接中的第二条订阅
  custom_proxy_group=v2ray`select`!!GROUP=V2RayProvider
  ```

- 现在也可以使用双条件进行筛选

  ```ini
  custom_proxy_group=g1hk`select`!!GROUPID=0!!(HGC|HKBN|PCCW|HKT|hk|港)
  # 订阅链接中的第一条订阅内名字含 HGC、HKBN、PCCW、HKT、hk、港 的节点
  ```

#### [server] 部分

> 此部分通常**保持默认**即可

1. **listen**

   > 绑定到 Web 服务器的地址，将地址设为 0.0.0.0，则局域网内设备均可使用。

1. **port**

   > 绑定到 Web 服务器地址的端口，默认为 25500

#### [advanced] 部分

> 此部分通常**保持默认**即可

## 自动上传

> 自动上传 gist ，可以用于 Clash For Android / Surge 等进行远程订阅

在程序目录内的 [gistconf.ini](./gistconf.ini) 中添加 [Personal Access Token](https://github.com/settings/tokens/new)，在链接后加上 `upload=true` 就会在更新好后自动上传 gist。
例如：

```ini
[common]
;uncomment the following line and enter your token to enable upload function
token = xxxxxxxxxxxxxxxxxxxxxxxx(你的 Personal Access Token)
```

# subconverter
> Utility to convert between various subscription format
>
> 用于在各种订阅格式之间转换的实用程序 

[![Build Status](https://travis-ci.com/tindy2013/subconverter.svg?branch=master)](https://travis-ci.com/tindy2013/subconverter)

## Usage | 使用说明
```
http://127.0.0.1:25500/%CATEGORY%?url=%URL_ENCODED_LINKS%
eg: - http://127.0.0.1:25500/clash?url=/proxy.list
	- http://127.0.0.1:25500/quanx?url=https%3A%2F%2Fwww%2Exyz%2Ecom%2F
```
* CATEGORY : clash | clashr | quan | quanx | ss | ssd | ssr | surge | surfboard | v2ray .
  * 如上是**subconverter**所支持转换的类别。
* URL_ENCODED :  [CyberChef](https://gchq.github.io/CyberChef/#recipe=URL_Encode(true) )
  * 进行网址编码时可以考虑使用如上网站。
* Use `|` to separate multiple links before URL Encode.  
  * 在进行使用网址编码前使用 `|` 分割多条订阅链接。
* For more preference options, please check `pref.ini`.
  * 更多高级设置请参照 `pref.ini`。

# subconverter

![Build Status](https://travis-ci.com/tindy2013/subconverter.svg?branch=master)  ![Release Status](https://img.shields.io/github/v/release/tindy2013/subconverter.svg)

> Utility to convert between various subscription format  

## Usage

```txt
http://127.0.0.1:25500/%CATEGORY%?url=%URL_ENCODED_LINKS%
eg: - http://127.0.0.1:25500/clash?url=/proxy.list
    - http://127.0.0.1:25500/quanx?url=https%3A%2F%2Fwww%2Exyz%2Ecom%2F
```

* CATEGORY : `clash` , `clashr` , `quan` , `quanx` , `ss` , `ssd` , `ssr` , `surge` , `surfboard` , `v2ray` .
* URL_ENCODED :  [CyberChef](https://gchq.github.io/CyberChef/#recipe=URL_Encode(true) ) or [URLEncoder](https://www.urlencoder.org/)
* Use `|` to separate multiple links before URL Encode.  
* For more preference options, please check `pref.ini`.

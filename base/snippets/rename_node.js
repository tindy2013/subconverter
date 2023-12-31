function rename(node) {
  var cnml_locations_en = [
    "Jiangsu",
    "Beijing",
    "Shanghai",
    "Guangzhou",
    "Shenzhen",
    "Hangzhou",
    "Changzhou",
    "Xuzhou",
    "Qingdao",
    "Ningbo",
    "Zhenjiang",
    "Chengdu",
    "Changsha",
    "Yunfu",
  ];
  var cnml_locations_cn = [
    "江苏",
    "北京",
    "上海",
    "广州",
    "深圳",
    "杭州",
    "常州",
    "徐州",
    "青岛",
    "宁波",
    "镇江",
    "成都",
    "长沙",
    "云浮",
  ];

  var is_included = function (string, keywords) {
    for (var i = 0; i < keywords.length; i++)
      if (string.toLowerCase().search(keywords[i].toLowerCase()) != -1)
        return true;
    return false;
  };

  var get_airport_name = function (hostname) {
    if (is_included(hostname, [".example0.", ".demo0."]))
      return "Example0cloud";
    if (is_included(hostname, [".example1.", ".demo1."]))
      return "Example1cloud";
    return "Unknown";
  };

  var get_node_level = function (remark) {
    if (is_included(remark, ["VIP2"])) return "1";
    return "0";
  };

  var get_traffic_times = function (remark) {
    var tmp = "";
    if (remark.match(/[x\*倍率]\d+/g)) tmp = remark.match(/[x\*倍率]\d+/g)[0];
    if (remark.match(/\d+[x\*倍率]/g)) tmp = remark.match(/\d+[x\*倍率]/g)[0];
    if (remark.match(/[x\*倍率]\d+\.\d+/g))
      tmp = remark.match(/[x\*倍率]\d+\.\d+/g)[0];
    if (remark.match(/\d+\.\d+[x\*倍率]/g))
      tmp = remark.match(/\d+\.\d+[x\*倍率]/g)[0];
    if (tmp == "") return "1.00";
    else return parseFloat(tmp.replace(/[^\d.]/gi, "")).toFixed(2);
  };

  var get_node_location = function (remark) {
    if (is_included(remark, ["Ascension", "阿森松"])) return "Ascension";
    if (is_included(remark, ["Argentina", "阿根廷"])) return "Argentina";
    if (is_included(remark, ["Austria", "Vienna", "奥地利", "维也纳"]))
      return "Austria";
    if (
      is_included(remark, ["Australia", "Sydney", "澳大利亚", "澳洲", "悉尼"])
    )
      return "Australia";
    if (is_included(remark, ["Belgium", "比利时"])) return "Belgium";
    if (is_included(remark, ["Brazil", "Paulo", "巴西", "圣保罗"]))
      return "Brazil";
    if (
      is_included(remark, [
        "Canada",
        "Montreal",
        "Vancouver",
        "加拿大",
        "蒙特利尔",
        "温哥华",
        "楓葉",
        "枫叶",
      ])
    )
      return "Canada";
    if (is_included(remark, ["Switzerland", "Zurich", "瑞士", "苏黎世"]))
      return "Switzerland";
    if (is_included(remark, ["Germany", "Frankfurt", "法兰克福", "德"]))
      return "Germany";
    if (is_included(remark, ["Denmark", "丹麦"])) return "Denmark";
    if (is_included(remark, ["Spain", "西班牙"])) return "Spain";
    if (is_included(remark, ["Europe", "欧洲"])) return "Europe";
    if (is_included(remark, ["Finland", "Helsinki", "芬兰", "赫尔辛基"]))
      return "Finland";
    if (is_included(remark, ["France", "Paris", "法国", "巴黎"]))
      return "France";
    if (
      is_included(remark, [
        "Indonesia",
        "Jakarta",
        "印尼",
        "印度尼西亚",
        "雅加达",
      ])
    )
      return "Indonesia";
    if (is_included(remark, ["Ireland", "Dublin", "爱尔兰", "都柏林"]))
      return "Ireland";
    if (is_included(remark, ["India", "Mumbai", "印度", "孟买"]))
      return "India";
    if (is_included(remark, ["Italy", "Milan", "意大利", "米兰"]))
      return "Italy";
    if (is_included(remark, ["NorthKorea", "朝鲜"])) return "North Korea";
    if (is_included(remark, ["Korea", "Seoul", "KOR", "首尔", "韩", "韓"]))
      return "Korea";
    if (is_included(remark, ["Macao", "澳门", "CTM"])) return "China Macao";
    if (is_included(remark, ["Malaysia", "马来西亚"])) return "Malaysia";
    if (is_included(remark, ["Netherlands", "Amsterdam", "荷兰", "阿姆斯特丹"]))
      return "Netherlands";
    if (is_included(remark, ["Philippines", "菲律宾"])) return "Philippines";
    if (is_included(remark, ["Romania", "罗马尼亚"])) return "Romania";
    if (is_included(remark, ["Arabia", "沙特"])) return "Arabia";
    if (is_included(remark, ["Dubai", "迪拜"])) return "Dubai";
    if (is_included(remark, ["Sweden", "瑞典"])) return "Sweden";
    if (is_included(remark, ["Thailand", "Bangkok", "泰国", "曼谷"]))
      return "Thailand";
    if (is_included(remark, ["Turkey", "Istanbul", "土耳其", "伊斯坦布尔"]))
      return "Turkey";
    if (is_included(remark, ["Vietnam", "越南"])) return "Vietnam";
    if (is_included(remark, ["Africa", "南非"])) return "South Africa";
    if (
      is_included(remark, [
        "UK",
        "England",
        "UnitedKingdom",
        "London",
        "英",
        "伦敦",
      ])
    )
      return "United Kingdom";
    if (
      is_included(remark, [
        "JP",
        "Japan",
        "Tokyo",
        "Osaka",
        "Saitama",
        "日本",
        "东京",
        "大阪",
        "埼玉",
        "日",
      ])
    )
      return "Japan";
    if (is_included(remark, ["SG", "Singapore", "新加坡", "狮城", "新"]))
      return "Singapore";
    if (
      is_included(remark, [
        "RU",
        "Russia",
        "Moscow",
        "Petersburg",
        "Siberia",
        "伯力",
        "莫斯科",
        "圣彼得堡",
        "西伯利亚",
        "新西伯利亚",
        "哈巴罗夫斯克",
        "俄罗斯",
        "俄",
      ])
    )
      return "Russia";
    if (
      is_included(remark, [
        "US",
        "America",
        "UnitedStates",
        "Portland",
        "Dallas",
        "Oregon",
        "Phoenix",
        "Fremont",
        "SiliconValley",
        "LasVegas",
        "LosAngeles",
        "SanJose",
        "SantaClara",
        "Seattle",
        "Chicago",
        "美国",
        "美",
        "波特兰",
        "达拉斯",
        "俄勒冈",
        "凤凰城",
        "费利蒙",
        "弗里蒙特",
        "硅谷",
        "拉斯维加斯",
        "洛杉矶",
        "圣何塞",
        "圣荷西",
        "圣克拉拉",
        "西雅图",
        "芝加哥",
      ])
    )
      return "United States";
    if (
      is_included(remark, [
        "TW",
        "Taiwan",
        "新北",
        "彰化",
        "CHT",
        "台",
        "HINET",
      ])
    )
      return "China Taiwan";
    if (
      is_included(remark, [
        "HK",
        "HongKong",
        "HKT",
        "HKBN",
        "HGC",
        "WTT",
        "CMI",
        "港",
      ])
    )
      return "China Hong Kong";
    if (
      is_included(remark, [
        "CN",
        "China",
        "Jiangsu",
        "Beijing",
        "Shanghai",
        "Guangzhou",
        "Shenzhen",
        "Hangzhou",
        "Changzhou",
        "Xuzhou",
        "Qingdao",
        "Ningbo",
        "Zhenjiang",
        "Chengdu",
        "Changsha",
        "Yunfu",
        "回国",
        "中国",
        "江苏",
        "北京",
        "上海",
        "广州",
        "深圳",
        "杭州",
        "常州",
        "徐州",
        "青岛",
        "宁波",
        "镇江",
        "成都",
        "长沙",
        "云浮",
        "back",
      ])
    )
      return "China Mainland";
    return "Unknown";
  };

  var get_forward_location = function (remark) {
    for (var i = 0; i < cnml_locations_en.length; i++) {
      if (is_included(remark, [cnml_locations_en[i]]))
        return " (" + cnml_locations_en[i] + " NAT)";
      if (is_included(remark, [cnml_locations_cn[i]]))
        return " (" + cnml_locations_en[i] + " NAT)";
    }
    return "";
  };

  var get_exact_location = function (remark, is_cnml) {
    var replace_from = [
      "Paulo",
      "Petersburg",
      "SiliconValley",
      "LasVegas",
      "LosAngeles",
      "SanJose",
      "SantaClara",
      "弗里蒙特",
      "圣荷西",
    ];
    var replace_to = [
      "Sao Paulo",
      "Saint Petersburg",
      "Silicon Valley",
      "Las Vegas",
      "Los Angeles",
      "San Jose",
      "Santa Clara",
      "Fremont",
      "San Jose",
    ];
    var locations_en = [
      "Vienna",
      "Sydney",
      "Montreal",
      "Vancouver",
      "Zurich",
      "Frankfurt",
      "Helsinki",
      "Paris",
      "London",
      "Jakarta",
      "Dublin",
      "Mumbai",
      "Milan",
      "Tokyo",
      "Osaka",
      "Saitama",
      "Seoul",
      "Amsterdam",
      "Moscow",
      "Saint Petersburg",
      "Siberia",
      "Bangkok",
      "Istanbul",
      "Portland",
      "Dallas",
      "Oregon",
      "Phoenix",
      "Fremont",
      "Silicon Valley",
      "Las Vegas",
      "Los Angeles",
      "San Jose",
      "Santa Clara",
      "Seattle",
      "Chicago",
      "Sao Paulo",
    ];
    var locations_cn = [
      "维也纳",
      "悉尼",
      "蒙特利尔",
      "温哥华",
      "苏黎世",
      "法兰克福",
      "赫尔辛基",
      "巴黎",
      "伦敦",
      "雅加达",
      "都柏林",
      "孟买",
      "米兰",
      "东京",
      "大阪",
      "埼玉",
      "首尔",
      "阿姆斯特丹",
      "莫斯科",
      "圣彼得堡",
      "西伯利亚",
      "曼谷",
      "伊斯坦布尔",
      "波特兰",
      "达拉斯",
      "俄勒冈",
      "凤凰城",
      "费利蒙",
      "硅谷",
      "拉斯维加斯",
      "洛杉矶",
      "圣何塞",
      "圣克拉拉",
      "西雅图",
      "芝加哥",
      "圣保罗",
    ];
    for (var i = 0; i < replace_from.length; i++)
      remark = remark.replace(replace_from[i], replace_to[i]);
    var forward_location = "";
    if (is_cnml) {
      locations_en = locations_en.concat(cnml_locations_en);
      locations_cn = locations_cn.concat(cnml_locations_cn);
    } else {
      forward_location = get_forward_location(remark);
    }
    for (var i = 0; i < locations_en.length; i++) {
      if (is_included(remark, [locations_en[i]]))
        return [locations_en[i], forward_location];
      if (is_included(remark, [locations_cn[i]]))
        return [locations_en[i], forward_location];
    }
    return ["", forward_location];
  };

  var get_node_features = function (hostname, remark, is_cnml) {
    var features = get_exact_location(remark, is_cnml);
    if (is_included(remark, ["BGP"]) || is_included(hostname, ["BGP"]))
      features[0] += " BGP";
    if (is_included(remark, ["IPLC"]) || is_included(hostname, ["IPLC"]))
      features[0] += " IPLC";
    if (is_included(remark, ["IEPL"]) || is_included(hostname, ["IEPL"]))
      features[0] += " IEPL";
    if (is_included(remark, ["CEN"]) || is_included(hostname, ["CEN"]))
      features[0] += " CEN";
    if (is_included(remark, ["AIA"]) || is_included(hostname, ["AIA"]))
      features[0] += " AIA";
    if (is_included(remark, ["AGA"]) || is_included(hostname, ["AGA"]))
      features[0] += " AGA";
    if (
      is_included(remark, ["Game", "Gaming", "游戏"]) ||
      is_included(hostname, ["Game", "Gaming", "游戏"])
    )
      features[0] += " Game";
    if (
      is_included(remark, [
        "Stream",
        "Media",
        "Netflix",
        "Unlock",
        "流",
        "媒体",
        "奈飞",
        "解锁",
      ])
    )
      features[0] += " Unlocked";
    return features[0].trim() + features[1];
  };

  var info = JSON.parse(node.ProxyInfo);
  var hostname = info.Hostname;
  var remark = info.Remark.replace(" ", "");
  return "[{airport_name}][L{node_level} (*{traffic_times})] {node_location} {node_features}"
    .replace("{airport_name}", get_airport_name(hostname))
    .replace("{node_level}", get_node_level(remark))
    .replace("{traffic_times}", get_traffic_times(remark))
    .replace("{node_location}", get_node_location(remark))
    .replace(
      "{node_features}",
      get_node_features(
        hostname,
        remark,
        is_included(get_node_location(remark), ["China Mainland"])
      )
    )
    .trim();
}

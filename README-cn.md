# subconverter

---

将自定义的配置、规则等放到 replacements 目录下，与 base 目录同结构。将 gist 文件 ID、GitHub Token、机场订阅地址 分别填入项目 secrets 对应变量 GIST_ID、GITHUB_TOKEN、URL。

Push 代码到 dev 分支，将会触发构建，并推送生成的规则配置到指定 gist 地址。

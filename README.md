# 电商领域搜索分词。

所有的代码才有纯C编写，字典用Redis存储。

Redis 字典的格式如下：

set 户 '{"0":"户外","1":"户型"}'

set 背 '{"0":"背包","1":"背景"}'

就像上面展示的，词条的首汉子作为键值存储在Redis中。

具体的代码处理在main.c有体现，有很详细的注释。

这个是项目截图。

 ![image](https://github.com/lokenetwork/chinese-participle/blob/master/demo-pictures/chinese-participle.png)
 
# 此项目后续开发计划。
1/3 做个后台管理词典，web方式，打算采用node跟一些前端MVC框架，angular之类。

2/3 支持mysql商品表导入，输入搜索词，返回分词匹配到的商品ID，支持分页返回。

3/3 开发错误搜索词纠正功能。

........

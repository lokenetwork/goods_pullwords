# 电商领域搜索分词。

The all code is write in clang.And the dictionary is store in Redis.

Redis dictionary format is as below:

set 户 '{"0":"户外","1":"户型"}'

set 背 '{"0":"背包","1":"背景"}'

as you see,The first chinese word is the key of Redis.

And the most process logic is in the "main.c".

These are some picture for project.

 ![image](https://github.com/lokenetwork/chinese-participle/blob/master/demo-pictures/chinese-participle.png)

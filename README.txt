config_parse
============

parse configfile
解析配置文件

===========
1. 【特性】
     * 本库支持多个setion
     * 键和值的分割,注释符，区限定符都支持自定义，默认以=分割，# 号注释，[]限定(注释暂时只支持行首注释，	在数据后面注释无效)
     * read_item中item为NULL直接返回错误，不允许此操作，section为NULL查找任意分区下第一个匹配到的item
 	同样，del时，section为NULL时默认删除所有section下的item项目，item=NULL删除整个section
 	同时，如果item=NULL或者item=“”直接返回, 不允许item为空字符串，这是大前提
     * 当区中item个数为0时不删除区信息，必须手动调用删除
     * 数据结构中，全局变量也就是没有区的数据也归于一个区，只不过输出时先输出
     * 由于使用了线程读写锁，因此需要加上-lpthread进行编译
     * 允许value值为空，但不允许item的值为空
     * 配置文件的各种标识符必须使用，否则不会生效，没有定义的标志一律用UNDEFINED标志，不能不使用
     * 使用了线程读写锁，但是可以在多进程中使用
     * 文件被修改后保存文件，非配置项会被丢弃(像空行或注释等)
     * 支持块注释，但块注释不能只注释一行,格式是：“注释符号+{” 和“注释符号+}”

==========
2. 【使用方法】

    char buf[100];
    config_rw_tool *conf = create_config_tool("conf.log");
    conf->init(conf, "cdn.conf", SQUARE_BRACKET, POUND);
    conf->get_item(conf, NULL, "SERVER", buf, 100);
    conf->close(conf);
    destroy_config_tool(conf);


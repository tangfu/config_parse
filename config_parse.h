/**
 * @file config_parse.h
 * @brief
 *
 *	配置文件读取库
 *
*
 * @note    C语言无法限制变量的访问权限，缺乏封装性；同时无法鉴定和区分对象，因此C不可能真正的面向对象
 *
 * @author tangfu - abctangfuqiang2008@163.com
 * @version 0.1
 * @date 2011-06-07
 */


#ifndef __CONFIG_FILE_H__
#define	__CONFIG_FILE_H__

#ifndef CFG_TRUE
#define CFG_TRUE 1
#endif

#ifndef CFG_FALSE
#define CFG_FALSE 0
#endif

#ifndef CFGBOOL
typedef int CFGBOOL;
#endif

#define CFG_BUFFER_LEN 1024
#define CFG_ITEM_LEN    128
#define CFG_FILE_LEN	128

#define _LARGEFILE_SOURCE
#define	_LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64


#include <stdio.h>
#include <pthread.h>
#include "list.h"


typedef enum cfg_type_s CFG_TYPE;
typedef enum section_type_s SECTION_TYPE;
typedef enum comment_type_s COMMENT_TYPE;
typedef enum separator_type_s SEPARATOR_TYPE;
typedef enum sort_style_s SORT_STYLE;

enum cfg_type_s {SECTION = 1, ITEM, VALUE};
enum sort_style_s { INC, DESC };
enum section_type_s {SECTION_UNDEFINED = 0, SQUARE_BRACKET = 1, ANGLE_BRACKET, LEADER_POUND };
enum comment_type_s {COMMENT_UNDEFINED = 0, POUND = '#', COMMA = ',', SEMICOLON = ';', COLON = ':', SINGLE_QUOT = '\'' };
enum separator_type_s {SEPARATOR_UNDEFINED = 0, EQUAL = '=', BLANK_OR_TAB = ' '};


#define SECTION_DEFAULT SQUARE_BRACKET
#define COMMENT_DEFAULT POUND
#define SEPARATOR_DEFAULT EQUAL

#define LINE_COMM 1
#define BLOCK_COMM 2



typedef struct config_item_h config_item;
typedef struct config_section_h config_section;
typedef struct config_header_h config_header;
typedef struct config_rw_tool_h config_rw_tool;
typedef struct config_property_h config_property;

struct config_property_h {
        SECTION_TYPE seg_type;
        SEPARATOR_TYPE sep_type;
        COMMENT_TYPE comm_type;
};


struct config_item_h {
        char item[CFG_ITEM_LEN];
        char value[CFG_ITEM_LEN];
        struct list_head list;
};


struct config_section_h {
        char section[CFG_ITEM_LEN];
        int item_cnt;
        struct list_head item_list;
        struct list_head list;
};

struct config_header_h {
        int item_cnt;
        int section_cnt;
        pthread_rwlock_t lock;
        struct list_head section_list;
};


struct config_rw_tool_h {
        /**
        * @brief init
        *
        * 完成库的初始化，每次初始化必然成功
        *
        * @param this		库对象指针
        * @param file		指向的配置文件
        * @param type		配置文件属性设定
        * @param test_conf	验证配置文件正确性的函数指针，需要用户自己送入
        *
        * @return		库的CFGBOOL值
        */
        CFGBOOL( *init )( config_rw_tool *this, const char *file, config_property *type, CFGBOOL( *test_conf )( void *fp ) );
        /**
        * @brief   save
        *
        * 将库对象保存回配置文件(可以单独指定配置文件)
        *
        * @param   this	    库对象指针
        * @param   new_file	    配置文件，为NULL时表示保存回打开的文件
        *
        * @return
        */
        CFGBOOL( *save )( config_rw_tool *this, const char *newfile );
        /**
        * @brief   load
        *
        * 重新加载配置文件
        *
        * @param   this			库对象指针
        * @param   test_conf	检测配置文件的函数指针
        *
        * @return
        */
        CFGBOOL( *load )( config_rw_tool *this, CFGBOOL( *test_conf )( void *fp ) );
        /**
        * @brief   conf_get_section_cnt
        *
        * 获取分区个数
        *
        * @param   this    库对象指针
        *
        * @return	分区个数
        */
        int ( *get_section_cnt )( config_rw_tool *this );
        /**
        * @brief   get_section_list_self
        *
        * 获取库对象本身维护的分区列表的链表头
        *
        * @param   this		库对象指针
        *
        * @attention	    修改获取到的这个链表中的数据回影响库对象本身
        *
        * @return	    库分区列表的链表头
        */
        struct list_head * ( *get_section_list_self )( config_rw_tool *this );
        /**
        * @brief   get_section_list
        *
        * 获取分区列表
        * @param   this				库对象指针
        * @param   section_list		分区列表链表头
        *
        * @attention 这个返回的分区列表与库对象是独立的，因为函数内部单独分配了空间
        *
        * @return
        *	成功    -	分区对应的记录条数\n
        *	出错	-	-1\n
        */
        int ( *get_section_list )( config_rw_tool *this, struct list_head *section_list );
        void ( *free_section_list )( struct list_head *section_list );
        /**
        * @brief   get_item
        *
        * 获取配置文件中的item项数据
        *
        * @param   this			库对象指针
        * @param   section		分区名称
        * @param   item			记录名称
        * @param   value		返回的记录值
        * @param   value_len	记录值buf长度
        *
        * @return  库CFGBOOL
        *
        * @attention
        *		section = NULL 代表读取任意的项目（item）值 有可能重复\n
        *		section !=NULL 代表读取section		* 所属的项目值
        */
        CFGBOOL( *get_item )( config_rw_tool *this, const char *section, const char *item, char *value, int value_len );
        /**
        * @brief   is_contain
        *
        * 测试配置文件中的特定数据是否存在
        *
        * @param   this    库对象指针
        * @param   type    待测试的数据类型
        * @param   data    待测试的数据值
        *
        * @return  库CFGBOOL
        */
        CFGBOOL( *is_contain )( config_rw_tool *this, CFG_TYPE type, const char *data );
        /**
        * @brief   get_item_cnt
        *
        * 获取库对象中记录条数
        * @param   this			库对象指针
        * @param   section	    分区名称，为NULL时表示获取总记录条数,否则表示分区对应的记录条数
        *
        * @return  记录条数(总记录或者某个分区的记录数)
        */
        int ( *get_item_cnt )( config_rw_tool *this, const char *section );
        /**
        * @brief   get_item_list_self
        *
        * 获取库对象本身维护的记录列表的链表头
        *
        * @param   this		库对象指针
        * @param   section	分区名称
        *
        * @return	    库特定分区对应的记录链表,若不指定分区则直接返回NULL
        */
        struct list_head * ( *get_item_list_self )( config_rw_tool *this, const char *section );
        /**
        * @brief   get_item_list
        *
        * 获取记录列表
        * @param   this			库对象指针
        * @param   section		分区名称
        * @param   item_list	返回的记录列表链表头
        *
        * @attention 这个返回的记录列表与库对象是独立的，因为函数内部单独分配了空间
        *
        * @return
        *	成功	-   返回记录列表中记录条数\n
        *	失败	-   返回-1\n
        *
        */
        int ( *get_item_list )( config_rw_tool *this, const char *section, struct list_head *item_list );
        void ( *free_item_list )( struct list_head *item_list );
        /**
        * @brief   add_record
        *
        * 添加一条记录到配置文件库对象中(此时并没有写入文件)
        * @param   this		库对象指针
        * @param   section	分区名称
        * @param   item		记录名称
        * @param   value	记录值
        *
        * @return  CFGBOOL
        */
        CFGBOOL( *add_record )( config_rw_tool *this, const char *section, const char *item, const char *value );
        /**
        * @brief   del_record
        *
        * 从库对象中删除一条记录
        *
        * @param   this		库对象指针
        * @param   section	分区名称
        * @param   item		记录名称
        *
        * @return  库CFGBOOL
        */
        CFGBOOL( *del_record )( config_rw_tool *this, const char *section, const char *item );
        /**
         * @brief	close
         *
         * 关闭库对象中所打开的配置文件
         *
         * @param	this	库对象指针
         */
        void ( *close )( config_rw_tool *this );
        /**
         * @brief	sort
         *
         * 给记录或分区链表排序
         *
         * @param	head		链表头
         * @param	cnt			链表中的节点数目
         * @param	style		排序类型，递增，递减等
         * @param	type		分区或记录
         *
         * @return	库的CFGBOOL值
         */
        CFGBOOL( *sort )( struct list_head *head, int cnt, SORT_STYLE style, CFG_TYPE type );

};


#ifdef __cplusplus
extern "C" {
#endif

        config_rw_tool *create_config_tool( char* log );
        void destroy_config_tool( config_rw_tool *this );
        inline CFGBOOL cfg_get_rdlock( config_rw_tool * );
        inline CFGBOOL cfg_get_wrlock( config_rw_tool * );
        inline CFGBOOL cfg_release_lock( config_rw_tool * );

#ifdef __cplusplus
}
#endif

#endif    /* __CONFIG_FILE_H__ */


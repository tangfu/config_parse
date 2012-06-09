#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "config_parse.h"
static struct tm currTM;
///定义一个数组类型
typedef struct array_s {
        long addr;
        int base;
} ARRAY;


///隐藏实现细节，扩展用户数据结构
typedef struct config_rw_tool_internal_h config_rw_tool_internal;
struct config_rw_tool_internal_h {

        CFGBOOL( *init )( config_rw_tool *this, const char *file, config_property *type, CFGBOOL( *test_conf )( FILE *fp ) );
        CFGBOOL( *save )( config_rw_tool *this, const char * );
        CFGBOOL( *load )( config_rw_tool *this, CFGBOOL( *test_conf )( FILE *fp ) );
        int ( *get_section_cnt )( config_rw_tool *this );
        /// get_section_list_self 返回的是库数据结构自身的链表位置，get_section_list 开辟了新的内存区域，不会对库数据结构自身造成影响
        struct list_head * ( *get_section_list_self )( config_rw_tool *this );
        int ( *get_section_list )( config_rw_tool *this, struct list_head *section_list );
        void ( *free_section_list )( struct list_head *section_list );
        CFGBOOL( *get_item )( config_rw_tool *this, const char *section, const char *item, char *value, int value_len );
        CFGBOOL( *is_contain )( config_rw_tool *this, CFG_TYPE type, const char *data );
        int ( *get_item_cnt )( config_rw_tool *this, const char *section );
        /// get_item_list_self 返回的是库数据结构自身的链表位置，get_item_list 开辟了新的内存区域，不会对库数据结构自身造成影响
        struct list_head * ( *get_item_list_self )( config_rw_tool *this, const char *section );
        int ( *get_item_list )( config_rw_tool *this, const char *section, struct list_head *item_list );
        void ( *free_item_list )( struct list_head *item_list );
        CFGBOOL( *add_record )( config_rw_tool *this, const char *section, const char *item, const char *value );
        CFGBOOL( *del_record )( config_rw_tool *this, const char *section, const char *item );
        void ( *close )( config_rw_tool *this );
        CFGBOOL( *sort )( struct list_head *head, int cnt, SORT_STYLE style, CFG_TYPE type );

        COMMENT_TYPE comm_type;
        SECTION_TYPE seg_type;
        SEPARATOR_TYPE sep_type;///	separator
        char *buf;
        char file[CFG_FILE_LEN];
        CFGBOOL ischanged;
        CFGBOOL init_state;
        config_header data;
        FILE *fp;
        FILE *fd_log;
};



static inline int string_is_digita( char *s );


static CFGBOOL conf_init( config_rw_tool *this, const char *file, config_property *type, CFGBOOL( *test_conf )( FILE *fp ) );
static CFGBOOL conf_save( config_rw_tool *this, const char *new_file );
static CFGBOOL conf_contain( config_rw_tool *this, CFG_TYPE type, const char *data );
static CFGBOOL conf_get_item( config_rw_tool *this, const char *section, const char *item, char *value, int value_len );
static CFGBOOL conf_add_record( config_rw_tool *this, const char *section, const char *item, const char *value );
static CFGBOOL conf_del_record( config_rw_tool *this, const char *section, const char *item );
static int conf_get_item_cnt( config_rw_tool *this, const char *section );
static int conf_get_item_list( config_rw_tool *this, const char *section, struct list_head *item_list );
static void conf_free_item_list( struct list_head *item_list );
static void conf_free_section_list( struct list_head *section_list );
static int conf_get_section_cnt( config_rw_tool *this );
static int conf_get_section_list( config_rw_tool *this, struct list_head *section_list );
static inline struct list_head* conf_get_item_list_self( config_rw_tool *conf, const char *section );
static inline struct list_head* conf_get_section_list_self( config_rw_tool *this );




static CFGBOOL conf_load( config_rw_tool *this, CFGBOOL( *test_conf )( FILE *fp ) );
static void printf_by_section( config_rw_tool_internal *this );
static void conf_clear( config_rw_tool_internal *this );
static void conf_release( config_rw_tool *this );
static inline int issection( char *line_buf, SECTION_TYPE type );
static inline int iscomment( char *line_buf, COMMENT_TYPE type );
inline static void get_current_time( struct tm *p );
inline static config_item* pack_item_value( SEPARATOR_TYPE type, char *line_buf );


static CFGBOOL sort_list( struct list_head *head, int cnt, SORT_STYLE style, CFG_TYPE type );


/**
 * @brief   create_config_tool
 *
 * 创建配置文件库对象
 *
 * @param   log			日志文件路径(可以为NULL)
 *
 * @return  配置文件库对象指针
 */
config_rw_tool *create_config_tool( char *log )
{
        config_rw_tool_internal *this;
        this = ( config_rw_tool_internal * )malloc( sizeof( config_rw_tool_internal ) );

        if( this == NULL ) {
                fprintf( stderr, "create < config_rw_tool > failed\n" );
                return NULL;
        }

        memset( this, 0, sizeof( config_rw_tool_internal ) );
        //init member function pointer
        this->init = conf_init;
        this->add_record = conf_add_record;
        this->get_item_cnt = conf_get_item_cnt;
        this->get_item_list = conf_get_item_list;
        this->free_item_list = conf_free_item_list;
        this->get_section_cnt = conf_get_section_cnt;
        this->get_section_list = conf_get_section_list;
        this->free_section_list = conf_free_section_list;
        this->del_record = conf_del_record;
        this->is_contain = conf_contain;
        this->get_item = conf_get_item;
        this->close = conf_release;
        this->load = conf_load;
        this->save = conf_save;
        this->sort = sort_list;
        this->get_section_list_self = conf_get_section_list_self;
        this->get_item_list_self = conf_get_item_list_self;
        //init member var
        this->comm_type = COMMENT_UNDEFINED;
        this->seg_type = SECTION_UNDEFINED;
        this->sep_type = SEPARATOR_UNDEFINED;
        this->data.item_cnt = 0;
        this->data.section_cnt = 0;
        pthread_rwlockattr_t attr;
        pthread_rwlockattr_init( &attr );
        pthread_rwlockattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
        pthread_rwlock_init( &this->data.lock, &attr );
        pthread_rwlockattr_destroy( &attr );
        INIT_LIST_HEAD( &this->data.section_list );
        this->ischanged = CFG_FALSE;
        this->init_state = CFG_FALSE;
        this->fd_log = ( log == NULL ) ? stderr : fopen( log, "w+" );

        if( this->fd_log == NULL ) {
                free( this );
                fprintf( stderr, "create < config_rw_tool > failed : log failed\n" );
                return NULL;
        }

        this->buf = ( char * )malloc( CFG_BUFFER_LEN );

        if( this->buf == NULL ) {
                get_current_time( &currTM );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]create < config_rw_tool > failed : buffer failed\n",			currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );

                if( this->fd_log != stderr )
                        fclose( this->fd_log );

                free( this );
                return NULL;
        }

        memset( this->buf, '\0', CFG_BUFFER_LEN );
        return ( config_rw_tool * )this;
}

/**
 * @brief   destroy_config_tool
 *
 * 销毁配置文件库对象指针
 *
 * @param   crt		库对象指针
 */
void destroy_config_tool( config_rw_tool *crt )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )crt;
        conf_release( crt );

        if( this != NULL ) {
                if( this->buf != NULL )
                        free( this->buf );

                if( this->fd_log != NULL && this->fd_log != stderr ) {
                        fclose( this->fd_log );
                        this->fd_log = NULL;
                }

                pthread_rwlock_destroy( &this->data.lock );
                free( this );
        }
}


/**
 * @brief conf_init
 *
 * 完成库的初始化，每次初始化必然成功
 *
 * @param conf		库对象指针
 * @param file		指向的配置文件
 * @param type		配置文件属性设定
 * @param test_conf	验证配置文件正确性的函数指针，需要用户自己送入
 *
 * @return		库的CFGBOOL值
 */
static CFGBOOL conf_init( config_rw_tool *conf, const char *file, config_property *type, CFGBOOL( *test_conf )( FILE *fp ) )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        /* conf_release(conf); */

        /** section_type 和 comment_type少部分会有冲突，使用时注意下，例如LEADER_POUND和POUND ,不过会写入日志中 */
        if( type->comm_type == POUND && type->seg_type == LEADER_POUND ) {
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]init < config_rw_tool > failed : both comment_type and section_type are '#'\n",			currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;
        }

        this->comm_type = type->comm_type;
        this->seg_type = type->seg_type;
        this->sep_type = type->sep_type;
        memset( this->file, '\0', CFG_FILE_LEN );
        strncpy( this->file, file , CFG_FILE_LEN );

        if( conf_load( conf, test_conf ) == CFG_FALSE ) {
                get_current_time( &currTM );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]conf_load failed\n",
                         currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;
        }

        this->ischanged = CFG_FALSE;
        this->init_state = CFG_TRUE;
        return CFG_TRUE;
}

/**
 * @brief   conf_contain
 *
 * 测试配置文件中的特定数据是否存在
 *
 * @param   conf    库对象指针
 * @param   type    待测试的数据类型
 * @param   data    待测试的数据值
 *
 * @return  库CFGBOOL
 */
static CFGBOOL conf_contain( config_rw_tool *conf, CFG_TYPE type, const char *data )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;

        if( data == NULL || strlen( data ) == 0 )
                return CFG_FALSE;

        config_section *tmp_section;
        LIST_HEAD( temp );

        switch( type ) {
                case ITEM:
                        return conf_get_item( conf, NULL, data, NULL, 0 );
                        /* break; */
                case SECTION:
                        list_for_each_entry( tmp_section, &this->data.section_list , list ) {
                                if( strncmp( tmp_section->section, data, CFG_BUFFER_LEN ) == 0 )
                                        return CFG_TRUE;
                        }
                        break;
                default:
                        break;
        }

        return CFG_FALSE;
}


/**
 * @brief   conf_get_item
 *
 * 获取配置文件中的item项数据
 *
 * @param   conf	库对象指针
 * @param   section	分区名称
 * @param   item	记录名称
 * @param   value	返回的记录值
 * @param   value_len	记录值buf长度
 *
 * @return  库CFGBOOL
 *
 * @attention	section = NULL 代表读取任意的项目（item）值 有可能重复\nsection !=NULL 代表读取section  * 所属的项目值
 */
static CFGBOOL conf_get_item( config_rw_tool *conf, const char *section, const char *item, char *value, int value_len )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;

        if( item == NULL ) {
                get_current_time( &currTM );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]item can not be NULL\n",
                         currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;		/* 因为每次只允许读取一条记录，因此必须限制，有可能不同区下含有相同的item  */
        }

        if( this->data.item_cnt == 0 )
                return CFG_FALSE;

        config_section *tmp_section;
        config_item *tmp_item;
        pthread_rwlock_rdlock( &this->data.lock );

        if( section != NULL ) {
                list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                        if( strncasecmp( tmp_section->section, section , CFG_ITEM_LEN ) == 0 ) {
                                list_for_each_entry( tmp_item, &tmp_section->item_list, list ) {
                                        if( strncasecmp( tmp_item->item, item, CFG_ITEM_LEN ) == 0 ) {
                                                if( value != NULL ) {
                                                        //                            strncpy(value, tmp_item->value, value_len);
                                                        memcpy( value, tmp_item->value, CFG_ITEM_LEN > value_len ? value_len : CFG_ITEM_LEN );
                                                        value[value_len - 1] = '\0';
                                                }

                                                pthread_rwlock_unlock( &this->data.lock );
                                                return CFG_TRUE;
                                        }
                                }
                        }
                }
        } else {
                list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                        list_for_each_entry( tmp_item, &tmp_section->item_list, list ) {
                                if( strncasecmp( tmp_item->item, item, CFG_ITEM_LEN ) == 0 ) {
                                        if( value != NULL ) {
                                                strncpy( value, tmp_item->value, value_len );
                                                memcpy( value, tmp_item->value, CFG_ITEM_LEN > value_len ? value_len : CFG_ITEM_LEN );
                                                value[value_len - 1] = '\0';
                                        }

                                        pthread_rwlock_unlock( &this->data.lock );
                                        return CFG_TRUE;
                                }
                        }
                }
        }

        pthread_rwlock_unlock( &this->data.lock );
        return CFG_FALSE;
}



/**
 * @brief   conf_add_record
 *
 * 添加一条记录到配置文件库对象中(此时并没有写入文件)
 * @param   conf	库对象指针
 * @param   section	分区名称
 * @param   item	记录名称
 * @param   value	记录值
 *
 * @return  CFGBOOL
 */
static CFGBOOL conf_add_record( config_rw_tool *conf, const char *section, const char *item, const char *value )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;

        if( this->init_state == CFG_FALSE ) {
                get_current_time( &currTM );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]Dont initialize conf_rw_tool\n",
                         currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;
        }

        if( value == NULL ) {
                get_current_time( &currTM );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]conf_add_item : value can not be NULL\n",
                         currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;
        }

        config_section *tmp_section;
        config_item *tmp_item;
        LIST_HEAD( temp );
        LIST_HEAD( tp );
        pthread_rwlock_wrlock( &this->data.lock );

        if( conf_get_item( conf, section, item, NULL, 0 ) == CFG_TRUE ) {
                list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                        if( strncasecmp( tmp_section->section, section , CFG_ITEM_LEN ) == 0 ) {
                                list_for_each_entry( tmp_item, &tmp_section->item_list, list ) {
                                        if( strncasecmp( tmp_item->item, item, CFG_ITEM_LEN ) == 0 ) {
                                                memset( tmp_item->value, '\0', CFG_ITEM_LEN );
                                                strncpy( tmp_item->value, value, CFG_ITEM_LEN );
                                                return CFG_TRUE;
                                        }
                                }
                        }
                }
        } else {
                tmp_item = malloc( sizeof( config_item ) );

                if( tmp_item == NULL ) {
                        get_current_time( &currTM );
                        fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]conf_add_item : malloc failed\n",
                                 currTM.tm_year + 1900,
                                 currTM.tm_mon + 1,
                                 currTM.tm_mday,
                                 currTM.tm_hour,
                                 currTM.tm_min,
                                 currTM.tm_sec );
                        return CFG_FALSE;
                }

                memset( tmp_item, '\0', sizeof( config_item ) );
                INIT_LIST_HEAD( &tmp_item->list );
                strncpy( tmp_item->value, value, CFG_ITEM_LEN );
                strncpy( tmp_item->item, item, CFG_ITEM_LEN );
                list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                        if( strncasecmp( tmp_section->section, section , CFG_ITEM_LEN ) == 0 ) {
                                list_add_tail( &tmp_item->list, &tmp_section->item_list );
                                ++tmp_section->item_cnt;
                                ++this->data.item_cnt;
                                return CFG_TRUE;
                        }
                }
                // new tmp_section
                tmp_section = malloc( sizeof( config_section ) );

                if( tmp_section == NULL ) {
                        get_current_time( &currTM );
                        fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]conf_add_item : malloc failed\n",
                                 currTM.tm_year + 1900,
                                 currTM.tm_mon + 1,
                                 currTM.tm_mday,
                                 currTM.tm_hour,
                                 currTM.tm_min,
                                 currTM.tm_sec );
                        free( tmp_item );
                        return CFG_FALSE;
                }

                memset( tmp_section, '\0', sizeof( config_section ) );
                INIT_LIST_HEAD( &tmp_section->list );
                INIT_LIST_HEAD( &tmp_section->item_list );
                strncpy( tmp_section->section, section, CFG_ITEM_LEN );
                list_add_tail( &tmp_item->list, &tmp_section->item_list );
                ++tmp_section->item_cnt;
                ++this->data.item_cnt;
                return CFG_TRUE;
                list_add_tail( &tmp_section->list, &this->data.section_list );
        }

        pthread_rwlock_unlock( &this->data.lock );
        this->ischanged = CFG_TRUE;
        return CFG_TRUE;
}


/**
 * @brief   conf_del_record
 *
 * 从库对象中删除一条记录
 *
 * @param   conf	库对象指针
 * @param   section	分区名称
 * @param   item	记录名称
 *
 * @return  库CFGBOOL
 */
static CFGBOOL conf_del_record( config_rw_tool *conf, const char *section, const char *item )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        config_section *tmp_section;
        config_item *tmp_item;
        LIST_HEAD( temp );
        LIST_HEAD( tp );
        int flag = 0;	/* normal  */

        if( ( section == NULL && item == NULL ) || ( item != NULL && strlen( item ) == 0 )
          )
                return CFG_FALSE;		    /*  该配置库不允许item为空，因此不存在item为空的项目，直接返回,区和item都为空，不允许操作*/
        else if( section == NULL && item != NULL )
                flag = 1;		/*删除任意section下的该项目*/
        else if( section != NULL && item == NULL )
                flag = 2;		/* 删除section区下的所有项目,包括seciton区信息 */

        if( this->init_state == CFG_FALSE ) {
                get_current_time( &currTM );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]Dont initialize conf_rw_tool\n",
                         currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;
        }

        pthread_rwlock_wrlock( &this->data.lock );

        switch( flag ) {
                case 0:
                        list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                                if( strncasecmp( tmp_section->section, section , CFG_ITEM_LEN ) == 0 ) {
                                        list_for_each_entry( tmp_item, &tmp_section->item_list, list ) {
                                                if( strncasecmp( tmp_item->item, item, CFG_ITEM_LEN ) == 0 ) {
                                                        list_del( &tmp_item->list );
                                                        free( tmp_item );
                                                        --tmp_section->item_cnt;
                                                        --this->data.item_cnt;
                                                        break;
                                                }
                                        }
                                }
                        }
                        break;
                case 1:
                        list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                                list_for_each_entry( tmp_item, &tmp_section->item_list, list ) {
                                        if( strncasecmp( tmp_item->item, item, CFG_ITEM_LEN ) == 0 ) {
                                                list_del( &tmp_item->list );
                                                free( tmp_item );
                                                --tmp_section->item_cnt;
                                                --this->data.item_cnt;
                                                break;
                                        }
                                }
                        }
                        break;
                case 2:
                        list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                                if( strncasecmp( tmp_section->section, section , CFG_ITEM_LEN ) == 0 ) {
                                        while( tmp_section->item_list.next != &tmp_section->item_list ) {
                                                tmp_item = list_entry( tmp_section->item_list.next, config_item, list );
                                                list_del( tmp_section->item_list.next );
                                                free( tmp_item );
                                                --tmp_section->item_cnt;
                                                --this->data.item_cnt;
                                        }
                                }
                        }
                        break;
                default:
                        break;
        }

        pthread_rwlock_unlock( &this->data.lock );
        this->ischanged = CFG_TRUE;
        return CFG_TRUE;
}



/**
 * @brief   conf_save
 *
 * 将库对象保存回配置文件(可以单独指定配置文件)
 *
 * @param   conf	    库对象指针
 * @param   new_file	    配置文件，为NULL时表示保存回打开的文件
 *
 * @return
 */
static CFGBOOL conf_save( config_rw_tool *conf, const char *new_file )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;

        if( new_file == NULL ) {
                if( this->init_state && this->ischanged ) {
                        conf_clear( this );

                        if( this->comm_type != COMMENT_UNDEFINED )
                                fprintf( this->fp, "%c\tconfig_rw_tool_lib	-	a  library which is used to read or write config file\n%c\t2010-11-27\tin\tuestc\n\n", this->comm_type, this->comm_type );

                        pthread_rwlock_wrlock( &this->data.lock );
                        printf_by_section( this );
                        pthread_rwlock_unlock( &this->data.lock );
                        rewind( this->fp );
                        this->ischanged = CFG_FALSE;
                }
        } else {
                FILE *temp, *fp = fopen( new_file, "w+" );

                if( fp == NULL ) {
                        fprintf( this->fp, "%s open failed\n", new_file );
                        return CFG_FALSE;
                }

                if( this->comm_type != COMMENT_UNDEFINED )
                        fprintf( this->fp, "%c\tconfig_rw_tool_lib	-	a  library which is used to read or write config file\n%c\t2010-11-27\tin\tuestc\n\n", this->comm_type, this->comm_type );

                pthread_rwlock_wrlock( &this->data.lock );
                temp = this->fp;
                this->fp = fp;
                printf_by_section( this );
                this->fp = temp;
                pthread_rwlock_unlock( &this->data.lock );
                fclose( fp );
        }

        return CFG_TRUE;
}




/**
 * @brief   conf_get_item_cnt
 *
 * 获取库对象中记录条数
 * @param   conf	    库对象指针
 * @param   section	    分区名称，为NULL时表示获取总记录条数,否则表示分区对应的记录条数
 *
 * @return  记录条数(总记录或者某个分区的记录数)
 */
static int conf_get_item_cnt( config_rw_tool *conf, const char *section )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        config_section *tmp_section;

        if( section == NULL )
                return this->data.item_cnt;
        else {
                pthread_rwlock_rdlock( &this->data.lock );
                list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                        if( strncmp( tmp_section->section, section, CFG_BUFFER_LEN ) == 0 ) {
                                pthread_rwlock_unlock( &this->data.lock );
                                return tmp_section->item_cnt;
                        }
                }
                pthread_rwlock_unlock( &this->data.lock );
        }

        return -1;
}

static void conf_free_item_list( struct list_head *item_list )
{
        config_item *tmp_item;

        while( item_list->next != item_list ) {
                tmp_item = list_entry( item_list->next, config_item, list );
                list_del( item_list->next );
                free( tmp_item );
        }
}

/**
 * @brief   conf_get_item_list
 *
 * 获取记录列表
 * @param   conf	库对象指针
 * @param   section	分区名称
 * @param   item_list	返回的记录列表链表头
 *
 * @attention 这个返回的记录列表与库对象是独立的，因为函数内部单独分配了空间
 *
 * @return
 *	成功	-   返回记录列表中记录条数\n
 *	失败	-   返回-1\n
 *
 */
static int conf_get_item_list( config_rw_tool *conf, const char *section, struct list_head *item_list )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        INIT_LIST_HEAD( item_list );
        config_section *tmp_section;
        config_item *tmp_item, *temp_item;
        int cnt = 0;

        if( section == NULL )
                return -1;
        else {
                pthread_rwlock_rdlock( &this->data.lock );
                list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                        if( strncmp( tmp_section->section, section, CFG_BUFFER_LEN ) == 0 ) {
                                list_for_each_entry( tmp_item, &tmp_section->item_list, list ) {
                                        temp_item = malloc( sizeof( config_item ) );

                                        if( temp_item == NULL ) {
                                                fprintf( stderr, "malloc failed in get item list,return\n" );
                                                pthread_rwlock_unlock( &this->data.lock );
                                                conf_free_item_list( item_list );
                                                return 0;
                                        }

                                        INIT_LIST_HEAD( &temp_item->list );
                                        memcpy( temp_item, tmp_item, sizeof( config_item ) );
                                        list_add_tail( &temp_item->list, item_list );
                                        ++cnt;
                                }
                                /* pthread_rwlock_unlock(&this->data.lock); */
                                break;
                                /* return &tmp_section->item_list; */
                        }
                }
                pthread_rwlock_unlock( &this->data.lock );
        }

        return cnt;
}


static void conf_free_section_list( struct list_head *section_list )
{
        config_section *tmp_section;

        while( section_list->next != section_list ) {
                tmp_section = list_entry( section_list->next, config_section, list );
                list_del( section_list->next );
                free( tmp_section );
        }
}


/**
 * @brief   conf_get_section_cnt
 *
 * 获取分区个数
 *
 * @param   conf    库对象指针
 *
 * @return	分区个数
 */
static int conf_get_section_cnt( config_rw_tool *conf )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        int cnt;
        pthread_rwlock_rdlock( &this->data.lock );
        cnt = this->data.section_cnt;
        pthread_rwlock_unlock( &this->data.lock );
        return cnt;
}


/**
 * @brief   conf_get_section_list
 *
 * 获取分区列表
 * @param   conf	    库对象指针
 * @param   section_list    分区列表链表头
 *
 * @attention 这个返回的分区列表与库对象是独立的，因为函数内部单独分配了空间
 *
 * @return
 *	成功    -	分区对应的记录条数\n
 *	出错	-	-1\n
 */
static int conf_get_section_list( config_rw_tool *conf, struct list_head *section_list )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        INIT_LIST_HEAD( section_list );
        config_section *tmp_section, *temp_section;
        int cnt = 0;
        pthread_rwlock_rdlock( &this->data.lock );
        list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                temp_section = malloc( sizeof( config_section ) );

                if( temp_section == NULL ) {
                        fprintf( stderr, "malloc failed in get section list,return\n" );
                        pthread_rwlock_unlock( &this->data.lock );
                        conf_free_section_list( section_list );
                        return -1;
                }

                memcpy( temp_section, tmp_section, sizeof( config_section ) );
                INIT_LIST_HEAD( &temp_section->list );
                //item_list 没有使用
                temp_section->item_list.next = NULL;
                temp_section->item_list.prev = NULL;
                list_add_tail( &temp_section->list, section_list );
                ++cnt;
        }
        pthread_rwlock_unlock( &this->data.lock );
        return cnt;
}


/* *************************************************************************
 *
 *		helper function
 *
 * **************************************************************************/
static void conf_clear( config_rw_tool_internal *this )
{
        if( this->fp != NULL )
                fclose( this->fp );

        char command[50];
        snprintf( command, 50, "mv %s .%s", this->file, this->file );
        system( command );
        this->fp = fopen( this->file, "w+" );
        this->ischanged = CFG_FALSE;
}

static void conf_release( config_rw_tool *conf )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        config_item *tmp_item;
        config_section *tmp_section;

        /* LIST_HEAD(temp);
        LIST_HEAD(tp); */
        if( this->init_state == CFG_TRUE ) {
                pthread_rwlock_wrlock( &this->data.lock );

                while( this->data.section_list.next != &this->data.section_list ) {
                        tmp_section = list_entry( this->data.section_list.next, config_section, list );

                        while( tmp_section->item_list.next != &tmp_section->item_list ) {
                                tmp_item = list_entry( tmp_section->item_list.next, config_item, list );
                                list_del( tmp_section->item_list.next );
                                --tmp_section->item_cnt;
                                --this->data.item_cnt;
                                free( tmp_item );
                        }

                        list_del( this->data.section_list.next );
                        --this->data.section_cnt;
                        free( tmp_section );
                }

                if( this->fp != NULL ) {
                        fclose( this->fp );
                        this->fp = NULL;
                }

                pthread_rwlock_unlock( &this->data.lock );
                /* this->item.itemno = 0; */
                /* this->item.segno = 0; */
                this->data.item_cnt = 0;
                this->data.section_cnt = 0;
                /* this->item.list_head=NULL; */
                this->init_state = CFG_FALSE;
                this->ischanged = CFG_FALSE;
        }
}

inline void print_section( SECTION_TYPE type, const config_section *conf_section, char *buf, int buflen )
{
        if( conf_section == NULL )
                return;

        const char *section = conf_section->section;

        switch( type ) {
                case SECTION_UNDEFINED:
                        /* by default,use SQUARE_BRACKE as section*/
                case SQUARE_BRACKET:
                        snprintf( buf, buflen, "[%s]", section );
                        break;
                case ANGLE_BRACKET:
                        snprintf( buf, buflen, "<%s>", section );
                        break;
                case LEADER_POUND:
                        snprintf( buf, buflen, "#%s", section );
                        break;
                default:	    /*没有定义的标识符，直接不予处理，即不打印*/
                        buf[0] = '\0';
                        break;
        }
}

inline void print_item( SEPARATOR_TYPE type, const config_item *conf_item, char *buf, int buflen )
{
        if( conf_item == NULL )
                return;

        switch( type ) {
                case SEPARATOR_UNDEFINED:
                        /* by default,use EQUAL as separator */
                case EQUAL:
                        snprintf( buf, buflen, "%s\t=\t%s", conf_item->item, conf_item->value );
                        break;
                case BLANK_OR_TAB:
                        snprintf( buf, buflen, "%s %s", conf_item->item, conf_item->value );
                        break;
                default:	/*没有定义的标识符，直接不予处理，即不打印*/
                        buf[0] = '\0';
                        break;
        }
}

static void printf_by_section( config_rw_tool_internal *this )
{
        config_section *tmp_section;
        config_item *tmp_item;
        list_for_each_entry_reverse( tmp_section, &this->data.section_list , list ) {
                print_section( this->seg_type, tmp_section, this->buf, CFG_BUFFER_LEN );
                fprintf( this->fp, "\n%s\n", this->buf );
                list_for_each_entry( tmp_item, &tmp_section->item_list, list ) {
                        print_item( this->sep_type, tmp_item, this->buf, CFG_BUFFER_LEN );
                        fprintf( this->fp, "%s\n", this->buf );
                        /* snprintf(this->buf,CFG_BUFFER_LEN,"%s\t=\t%s",tmp_item->item,tmp_item->value);
                        fprintf(this->fp,"%s\n",this->buf); */
                }
        }
}

/**
 * @brief   conf_load
 *
 * 重新加载配置文件
 *
 * @param   conf	库对象指针
 * @param   test_conf	检测配置文件的函数指针
 *
 * @return
 */
static CFGBOOL conf_load( config_rw_tool *conf, CFGBOOL( *test_conf )( FILE *fp ) )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        conf_release( conf );
        this->ischanged = CFG_FALSE;
        config_section *temp_section;
        config_item *temp_item;
        char *p, *temp;
        int block_flag = 0; /* 0:block comm shutdown;1:block comm open */
        /* rewind(this->fp); */
        this->fp = fopen( this->file, "a+" );

        if( this->fp == NULL ) {
                get_current_time( &currTM );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]init < config_rw_tool > failed : conf_file open failed\n",			currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;
        }

        if( test_conf != NULL )
                if( test_conf( this->fp ) == CFG_FALSE )
                        return CFG_FALSE;

        //fseek(fp, this->start, SEEK_SET);
        config_section *global_section;/* 全局变量没有单独算一个区  */
        global_section = malloc( sizeof( config_section ) );

        if( global_section == NULL ) {
                fprintf( stderr, "config_section malloc failed\n" );
                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]config_section malloc failed\n",
                         currTM.tm_year + 1900,
                         currTM.tm_mon + 1,
                         currTM.tm_mday,
                         currTM.tm_hour,
                         currTM.tm_min,
                         currTM.tm_sec );
                return CFG_FALSE;
        }

        temp_section = global_section;
        memset( temp_section, 0, sizeof( config_section ) );
        INIT_LIST_HEAD( &temp_section->list );
        INIT_LIST_HEAD( &temp_section->item_list );
        pthread_rwlock_wrlock( &this->data.lock );

        while( fgets( this->buf, CFG_BUFFER_LEN, this->fp ) != NULL ) {
                /* 预防一行特别长的情况，我们直接将最后两个字符给限定，如果超出了只是配置项的值会有错，程序运行不会出错 */
                this->buf[CFG_BUFFER_LEN - 2] = '\n';
                this->buf[CFG_BUFFER_LEN - 1] = '\0';

                if( strcmp( this->buf, "\n" ) == 0 || strcmp( this->buf, "\r\n" ) == 0 || strcmp( this->buf, "\r" ) == 0 )
                        continue;

                block_flag = iscomment( this->buf, this->comm_type );

                if( block_flag == LINE_COMM ) {	/* 如果是注释则进入下一循环 */
                        memset( this->buf, '\0', CFG_BUFFER_LEN );
                        continue;
                } else if( block_flag == BLOCK_COMM ) {
                        p = this->buf;

                        while( fgets( this->buf, CFG_BUFFER_LEN, this->fp ) != NULL ) {
                                this->buf[CFG_BUFFER_LEN - 2] = '\n';
                                this->buf[CFG_BUFFER_LEN - 1] = '\0';

                                if( strcmp( this->buf, "\n" ) == 0 || strcmp( this->buf, "\r\n" ) == 0 || strcmp( this->buf, "\r" ) == 0 )
                                        continue;

                                while( isspace( *p ) )
                                        ++p;

                                temp = strchr( p, this->comm_type );

                                if( temp == NULL )
                                        continue;
                                else {
                                        if( *( temp + 1 ) == '}' ) {
                                                block_flag = 0;
                                                break;
                                        } else
                                                continue;
                                }
                        }

                        if( block_flag == 0 )
                                continue;   //块注释完成
                        else
                                break;	//配置文件分析完了
                }

                if( issection( this->buf, this->seg_type ) == CFG_TRUE ) {
                        if( strlen( this->buf ) >= CFG_ITEM_LEN ) {
                                /* section部分超过ITEM_LEN，将其丢弃 */
                                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]section len beyond CFG_ITEM_LEN,drop it\n",
                                         currTM.tm_year + 1900,
                                         currTM.tm_mon + 1,
                                         currTM.tm_mday,
                                         currTM.tm_hour,
                                         currTM.tm_min,
                                         currTM.tm_sec );
                                continue;
                        }

                        temp_section = malloc( sizeof( config_section ) );

                        if( temp_section == NULL ) {
                                fprintf( this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]section malloc failed, exit\n",
                                         currTM.tm_year + 1900,
                                         currTM.tm_mon + 1,
                                         currTM.tm_mday,
                                         currTM.tm_hour,
                                         currTM.tm_min,
                                         currTM.tm_sec );
                                pthread_rwlock_unlock( &this->data.lock );
                                return CFG_FALSE;
                        }

                        memset( temp_section, '\0', sizeof( config_section ) );
                        INIT_LIST_HEAD( &temp_section->list );
                        INIT_LIST_HEAD( &temp_section->item_list );
                        strncpy( temp_section->section, this->buf , strlen( this->buf ) );
                        /* temp_section->section[strlen(this->buf) - 2] = '\0'; */
                        list_add_tail( &temp_section->list, &this->data.section_list );
                        ++this->data.section_cnt;
                        continue;
                }

                if( ( temp_item = pack_item_value( this->sep_type, this->buf ) ) != NULL ) {
                        ++temp_section->item_cnt;
                        ++this->data.item_cnt;
                        list_add_tail( &temp_item->list, &temp_section->item_list );
                }
        }

        if( global_section->item_cnt > 0 ) {
                list_add_tail( &global_section->list, &this->data.section_list );
        } else {
                free( global_section );
        }

        pthread_rwlock_unlock( &this->data.lock );
        return CFG_TRUE;
}


static inline int issection( char *line_buf, SECTION_TYPE type )
{
        int len = strlen( line_buf );
        char *tmp = line_buf;
        char *locate = line_buf + len - 1;
        int i = 0;

        if( strlen( line_buf ) <= 3 )
                return CFG_FALSE;  /* 只包含区限定符的行，以及换行符等等,不将其看作一个区 */

        /* while((locate = strchr(line_buf,'\r')))
            *locate = '\0';
        while((locate = strchr(line_buf,'\n')))
            *locate = '\0'; */
        while( isspace( *tmp ) != 0 ) ++tmp;

        while( isspace( *locate ) != 0 ) --locate;

        switch( type ) {
                case SECTION_UNDEFINED:/*默认使用SQUARE——BRACKET  */
                case SQUARE_BRACKET:

                        if( *tmp == '[' && *locate == ']' ) {
                                while( i < locate - tmp - 1 ) {
                                        *( line_buf + i ) = *( tmp + 1 + i );
                                        ++i;
                                }

                                *( line_buf + ( locate - tmp - 1 ) ) = '\0';
                                return CFG_TRUE;
                        }

                        break;
                case ANGLE_BRACKET:

                        if( *tmp == '<' && *locate == '>' ) {
                                while( i < locate - tmp - 1 ) {
                                        *( line_buf + i ) = *( tmp + 1 + i );
                                        ++i;
                                }

                                *( line_buf + ( locate - tmp - 1 ) ) = '\0';
                                return CFG_TRUE;
                        }

                        break;
                case LEADER_POUND:

                        if( *tmp == '#' ) {
                                while( i < locate - tmp ) {
                                        *( line_buf + i ) = *( tmp + 1 + i );
                                        ++i;
                                }

                                *( line_buf + ( locate - tmp ) ) = '\0';
                                return CFG_TRUE;
                        }

                default:
                        break;
        }

        return CFG_FALSE;
}

//块注释不能只注释一行，必须跨行
static inline int iscomment( char *line_buf, COMMENT_TYPE type )
{
        // int len = strlen(line_buf);
        char *temp = NULL, *p = line_buf;

        while( isspace( *p ) ) ++p;

        switch( type ) {
                case POUND:

                        //行首注释
                        if( *p == '#' ) {
                                if( *( p + 1 ) != '{' )
                                        return LINE_COMM;
                                else
                                        return BLOCK_COMM;
                        }

                        //行中注释
                        if( ( temp = strchr( p, '#' ) ) != NULL ) {
                                if( *( temp + 1 ) != '{' ) {
                                        *temp = '\n';
                                        *( temp + 1 ) = '\0';
                                } else {
                                        return BLOCK_COMM;
                                }
                        }

                        break;
                case COMMA:

                        if( *p == ',' ) {
                                if( *( p + 1 ) != '{' )
                                        return LINE_COMM;
                                else
                                        return BLOCK_COMM;
                        }

                        if( ( temp = strchr( p, ',' ) ) != NULL ) {
                                if( *( temp + 1 ) != '{' ) {
                                        *temp = '\n';
                                        *( temp + 1 ) = '\0';
                                } else {
                                        return BLOCK_COMM;
                                }
                        }

                        break;
                case SEMICOLON:

                        if( *p == ';' ) {
                                if( *( p + 1 ) != '{' )
                                        return LINE_COMM;
                                else
                                        return BLOCK_COMM;
                        }

                        if( ( temp = strchr( p, ';' ) ) != NULL ) {
                                if( *( temp + 1 ) != '{' ) {
                                        *temp = '\n';
                                        *( temp + 1 ) = '\0';
                                } else
                                        return BLOCK_COMM;
                        }

                        break;
                case COLON:

                        if( *p == ':' ) {
                                if( *( p + 1 ) != '{' )
                                        return LINE_COMM;
                                else
                                        return BLOCK_COMM;
                        }

                        if( ( temp = strchr( p, ':' ) ) != NULL ) {
                                if( *( temp + 1 ) != '{' ) {
                                        *temp = '\n';
                                        *( temp + 1 ) = '\0';
                                } else
                                        return BLOCK_COMM;
                        }

                        break;
                case SINGLE_QUOT:

                        if( *p == '\'' ) {
                                if( *( p + 1 ) != '{' )
                                        return LINE_COMM;
                                else
                                        return BLOCK_COMM;
                        }

                        if( ( temp = strchr( p, '\'' ) ) != NULL ) {
                                if( *( temp + 1 ) != '{' ) {
                                        *temp = '\n';
                                        *( temp + 1 ) = '\0';
                                } else
                                        return BLOCK_COMM;
                        }

                        break;
                default:
                        break;
        }

        return CFG_FALSE;
}

inline static config_item* pack_item_value( SEPARATOR_TYPE type, char *line_buf )
{
        char *tmp = line_buf;
        char *locate, *pmove;
        int item_len, value_len;
        config_item *temp_item;

        while( isspace( *tmp ) != 0 ) tmp++;

        switch( type ) {
                case SEPARATOR_UNDEFINED:    /* 默认采用=这种分割方式 */
                case EQUAL:
                        locate = strchr( tmp, '=' );		/* 暂时用=来代表数据项与值之间的关系 */

                        if( locate == NULL ) {
                                memset( line_buf, '\0', CFG_BUFFER_LEN );
                                return NULL;
                        }

                        pmove = locate;
                        --pmove;

                        while( isspace( *pmove ) != 0 ) --pmove;

                        item_len = pmove - tmp + 1;
                        *( pmove + 1 ) = '\0';
                        ++locate;

                        while( isspace( *locate ) != 0 ) ++locate;

                        pmove = locate;

                        while( isspace( *pmove ) == 0 ) ++pmove;

                        value_len = pmove - locate;
                        *pmove = '\0';

                        if( value_len <= 0 || item_len <= 0 ) { /* item或者value不允许为空 */
                                memset( line_buf, '\0', CFG_BUFFER_LEN );
                                return NULL;
                        }

                        break;
                case BLANK_OR_TAB:
                        pmove = tmp;

                        while( isspace( *pmove ) == 0 ) ++pmove;

                        locate = pmove;
                        item_len = pmove - tmp;

                        while( isblank( *pmove ) != 0 ) ++pmove;

                        if( pmove == locate ) {	/* 说明item和value之间的分隔符不包含空格和tab  */
                                memset( line_buf, '\0', CFG_BUFFER_LEN );
                                return NULL;
                        }

                        *locate = '\0';
                        locate = pmove;

                        while( isspace( *pmove ) == 0 ) ++pmove;

                        value_len = pmove - locate;
                        *pmove = '\0';

                        if( value_len <= 0 || item_len <= 0 ) { /* item或者value不允许为空 */
                                memset( line_buf, '\0', CFG_BUFFER_LEN );
                                return NULL;
                        }

                        break;
                default:		/* 不进行处理，因为没有定义这种标志 */
                        memset( line_buf, '\0', CFG_BUFFER_LEN );
                        return NULL;
        }

        temp_item = ( config_item * )malloc( sizeof( config_item ) );

        if( temp_item == NULL ) {
                /*
                	get_current_time(&currTM);
                	fprintf(this->fd_log, "[%d/%02d/%02d %02d:%02d:%02d]malloc config_item failed ,exit\n",
                		currTM.tm_year+1900,
                		currTM.tm_mon+1,
                		currTM.tm_mday,
                		currTM.tm_hour,
                		currTM.tm_min,
                		currTM.tm_sec); */
                memset( line_buf, '\0', CFG_BUFFER_LEN );
                return NULL;
        }

        memset( temp_item, '\0', sizeof( config_item ) );
        INIT_LIST_HEAD( &temp_item->list );
        strncpy( temp_item->item, tmp, CFG_ITEM_LEN );
        temp_item->item[CFG_ITEM_LEN - 1] = '\0';
        strncpy( temp_item->value, locate, CFG_ITEM_LEN );
        temp_item->value[CFG_ITEM_LEN - 1] = '\0';
        return temp_item;
}

inline static void get_current_time( struct tm *p )
{
        time_t tval;
        time( &tval );
        localtime_r( &tval, p );
}

static inline int string_is_digita( char *s )
{
        char *tmp = NULL;

        for( tmp = s; *tmp != '\0'; ++tmp ) {
                if( *tmp > 57 || *tmp < 48 )
                        return 0;
        }

        return 1;
}


/* 自己去枷锁，排序函数本身不进行枷锁 */
/* 只针对所排序的域是数字的情况，字符串不进行排序 */
static CFGBOOL sort_list( struct list_head *head, int cnt, SORT_STYLE style, CFG_TYPE type )
{
        ARRAY *list_index = malloc( cnt * sizeof( ARRAY ) );
        ARRAY swap;
        config_section *temp_section;
        config_item *temp_item;

        if( list_index == NULL ) {
                fprintf( stderr, "malloc failed\n" );
                return CFG_FALSE;
        }

        memset( list_index, 0, cnt * sizeof( ARRAY ) );
        int i = 0, j = 0 , temp;

        switch( type ) {
                case SECTION:
                        list_for_each_entry( temp_section, head, list ) {
                                if( string_is_digita( temp_section->section ) == 0 ) {
                                        fprintf( stderr, "index_value style is not true\n" );
                                        free( list_index );
                                        return CFG_FALSE;
                                }

                                list_index[i].addr = ( long )( &temp_section->list );
                                list_index[i++].base = atoi( temp_section->section );
                        }

                        switch( style ) {
                                case INC:

                                        while( --i ) {
                                                temp = i;

                                                for( j = 0; j < i; ++j ) {
                                                        if( list_index[j].base > list_index[temp].base )
                                                                temp = j;	// 记录最大值
                                                }

                                                if( temp != i ) {
                                                        swap = list_index[i];
                                                        list_index[i] = list_index[temp];
                                                        list_index[temp] = swap;
                                                        list_exchange( ( struct list_head * )( list_index[i].addr ), ( struct list_head * )( list_index[temp].addr ) );
                                                }

                                                /* list_move(&(struct list_head *)(list_index[temp]->addr), head); */
                                        }

                                        break;
                                case DESC:

                                        while( --i ) {
                                                temp = i;

                                                for( j = 0; j < i; ++j ) {
                                                        if( list_index[j].base < list_index[temp].base )
                                                                temp = j;	// 记录最小值
                                                }

                                                if( temp != i ) {
                                                        swap = list_index[i];
                                                        list_index[i] = list_index[temp];
                                                        list_index[temp] = swap;
                                                        list_exchange( ( struct list_head * )( list_index[i].addr ), ( struct list_head * )( list_index[temp].addr ) );
                                                }

                                                /* list_move(&(struct list_head *)(list_index[temp]->addr), head); */
                                        }

                                        break;
                                default:
                                        break;
                        }

                        break;
                case ITEM:
                        list_for_each_entry( temp_item, head, list ) {
                                if( string_is_digita( temp_item->item ) == 0 ) {
                                        fprintf( stderr, "index_value style is not true\n" );
                                        free( list_index );
                                        return CFG_FALSE;
                                }

                                list_index[i].addr = ( long )( &temp_item->list );
                                list_index[i++].base = atoi( temp_item->item );
                        }

                        switch( style ) {
                                case INC:

                                        while( --i ) {
                                                temp = i;

                                                for( j = 0; j < i; ++j ) {
                                                        if( list_index[j].base > list_index[temp].base )
                                                                temp = j;	// 记录最大值
                                                }

                                                if( temp != i ) {
                                                        swap = list_index[i];
                                                        list_index[i] = list_index[temp];
                                                        list_index[temp] = swap;
                                                        list_exchange( ( struct list_head * )( list_index[i].addr ), ( struct list_head * )( list_index[temp].addr ) );
                                                }

                                                /* list_move(&(struct list_head *)(list_index[temp]->addr), head); */
                                        }

                                        break;
                                case DESC:

                                        while( --i ) {
                                                temp = i;

                                                for( j = 0; j < i; ++j ) {
                                                        if( list_index[j].base < list_index[temp].base )
                                                                temp = j;	// 记录最小值
                                                }

                                                if( temp != i ) {
                                                        swap = list_index[i];
                                                        list_index[i] = list_index[temp];
                                                        list_index[temp] = swap;
                                                        list_exchange( ( struct list_head * )( list_index[i].addr ), ( struct list_head * )( list_index[temp].addr ) );
                                                }
                                        }

                                        break;
                                default:
                                        break;
                        }

                        break;
                case VALUE:
                        list_for_each_entry( temp_item, head, list ) {
                                if( string_is_digita( temp_item->value ) == 0 ) {
                                        fprintf( stderr, "index_value style is not true\n" );
                                        free( list_index );
                                        return CFG_FALSE;
                                }

                                list_index[i].addr = ( long )( &( temp_item->list ) );
                                list_index[i++].base = atoi( temp_item->value );
                        }

                        switch( style ) {
                                case INC:

                                        while( --i ) {
                                                temp = i;

                                                for( j = 0; j < i; ++j ) {
                                                        if( list_index[j].base > list_index[temp].base )
                                                                temp = j;	// 记录最大值
                                                }

                                                if( temp != i ) {
                                                        swap = list_index[i];
                                                        list_index[i] = list_index[temp];
                                                        list_index[temp] = swap;
                                                        list_exchange( ( struct list_head * )( list_index[i].addr ), ( struct list_head * )( list_index[temp].addr ) );
                                                }

                                                /* list_move(&(struct list_head *)(list_index[temp]->addr), head); */
                                        }

                                        break;
                                case DESC:

                                        while( --i ) {
                                                temp = i;

                                                for( j = 0; j < i; ++j ) {
                                                        if( list_index[j].base < list_index[temp].base )
                                                                temp = j;	// 记录最小值
                                                }

                                                if( temp != i ) {
                                                        swap = list_index[i];
                                                        list_index[i] = list_index[temp];
                                                        list_index[temp] = swap;
                                                        list_exchange( ( struct list_head * )( list_index[i].addr ), ( struct list_head * )( list_index[temp].addr ) );
                                                }
                                        }

                                        break;
                                default:
                                        break;
                        }
        }

        free( list_index );
        return CFG_TRUE;
}

/**
 * @brief   cfg_get_rdlock
 *
 * 给整个库上读琐(不会阻塞，获取不到则返回错误)
 *
 * @param   conf	库对象指针
 *
 * @return
 */
inline CFGBOOL cfg_get_rdlock( config_rw_tool *conf )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        return ( pthread_rwlock_tryrdlock( &this->data.lock ) == 0 ) ? CFG_TRUE : CFG_FALSE;
}
/**
 * @brief   cfg_get_wrlock
 *
 * 给整个库上写琐(不会阻塞，获取不到则返回错误)
 *
 * @param   conf	库对象指针
 *
 * @return
 */
inline CFGBOOL cfg_get_wrlock( config_rw_tool *conf )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        return ( pthread_rwlock_trywrlock( &this->data.lock ) == 0 ) ? CFG_TRUE : CFG_FALSE;
}
/**
 * @brief   cfg_release_lock
 *
 * 释放库琐
 *
 * @param   conf	库对象指针
 *
 * @return
 */
inline CFGBOOL cfg_release_lock( config_rw_tool *conf )
{
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        return ( pthread_rwlock_unlock( &this->data.lock ) == 0 ) ? CFG_TRUE : CFG_FALSE;
}

/**
 * @brief   conf_get_section_list_self
 *
 * 获取库对象本身维护的分区列表的链表头
 *
 * @param   this	库对象指针
 *
 * @attention	    修改获取到的这个链表中的数据回影响库对象本身
 *
 * @return	    库分区列表的链表头
 */
static inline struct list_head* conf_get_section_list_self( config_rw_tool *this ) {
        return &( ( config_rw_tool_internal * )this )->data.section_list;
}

/**
 * @brief   conf_get_item_list_self
 *
 * 获取库对象本身维护的记录列表的链表头
 *
 * @param   conf
 * @param   section	分区名称
 *
 * @return	    库特定分区对应的记录链表,若不指定分区则直接返回NULL
 */
static inline struct list_head* conf_get_item_list_self( config_rw_tool *conf, const char *section ) {
        config_rw_tool_internal *this = ( config_rw_tool_internal * )conf;
        config_section *tmp_section;

        if( section == NULL )
                return NULL;
        else {
                pthread_rwlock_rdlock( &this->data.lock );
                list_for_each_entry( tmp_section, &this->data.section_list, list ) {
                        if( strncmp( tmp_section->section, section, CFG_BUFFER_LEN ) == 0 ) {
                                pthread_rwlock_unlock( &this->data.lock );
                                return &tmp_section->item_list;
                        }
                }
                pthread_rwlock_unlock( &this->data.lock );
        }

        return NULL;
}


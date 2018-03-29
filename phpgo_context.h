#pragma once
#include <libgo/freeable.h>
#include <libgo/task_local_storage.h>

using namespace co;

#ifdef ZTS
	#define TSRMLS_FIELD TSRMLS_D
#else
	#define TSRMLS_FIELD
#endif

#ifdef ZTS
    /*save_to_ctx->tsrm_ls = tsrm_ls*/
	#define PHPGO_SAVE_TSRMLS(save_to_ctx) save_to_ctx->TSRMLS_C = TSRMLS_C
	
	/*void*** tsrm_ls = load_from_ctx->trsm_ls*/
	#define PHPGO_LOAD_TSRMLS(load_from_ctx) TSRMLS_C = load_from_ctx->TSRMLS_C
#else
	#define PHPGO_SAVE_TSRMLS(save_to_ctx)
	#define PHPGO_LOAD_TSRMLS(load_from_ctx)
#endif

#define NUM_TRACK_VARS	6

#define GET_HTTP_GLOBAL(name, http_globals, offset) \
do{ \
	zval** ppz_arr = nullptr; \
	zend_hash_find(&EG(symbol_table), name, sizeof(name), (void**)&ppz_arr); \
	if(ppz_arr) { \
		(http_globals)[offset] = *ppz_arr; \
		Z_ADDREF_P(*ppz_arr); \
	}else{ \
		(http_globals)[offset] = nullptr; \
	} \
}while(0)

#define GET_HTTP_REQUEST_GLOBAL(http_request_global) \
do{ \
	zval** ppz_arr = nullptr; \
	zend_hash_find(&EG(symbol_table), "_REQUEST", sizeof("_REQUEST"), (void**)&ppz_arr); \
	if(ppz_arr){ \
		http_request_global = *ppz_arr; \
		Z_ADDREF_P(*ppz_arr); \
	}else{ \
		http_request_global = nullptr; \
	} \
}while(0)

#define SET_HTTP_GLOBAL(name, http_globals, offset) \
do{ \
	if( http_globals[offset] ) {\
		zend_hash_update(&EG(symbol_table), name, sizeof(name), &(http_globals[offset]), sizeof(zval *), NULL); \
	} \
}while(0)

#define SET_HTTP_REQUEST_GLOBAL(http_request_global) \
do{ \
	if( http_request_global ) {\
		zend_hash_update(&EG(symbol_table), "_REQUEST", sizeof("_REQUEST"), &http_request_global, sizeof(zval *), NULL); \
	} \
}while(0)

struct PhpgoBaseContext{
	uint64_t                   go_routine_options;
	
	/*go routine running environment*/
	TSRMLS_FIELD;  /*ZTS: void ***tsrm_ls;*/
	struct _zend_execute_data* EG_current_execute_data; 
	zend_vm_stack 			   EG_argument_stack;       
	zend_class_entry*		   EG_scope;                
	zval*					   EG_This;                 
	zend_class_entry*		   EG_called_scope;         
	HashTable*				   EG_active_symbol_table;  
	zval**					   EG_return_value_ptr_ptr; 
	zend_op_array*			   EG_active_op_array;      
	zend_op**				   EG_opline_ptr;           
	zval                       EG_error_zval;           
	zval*                      EG_error_zval_ptr;       
	zval*                      EG_user_error_handler;   
	zval*                      PG_http_globals[NUM_TRACK_VARS];
	zval*                      http_request_global;
	/**/
	
public:
	inline void SwapOut(bool include_http_globals){
		TSRMLS_FIELD;                  
		PHPGO_LOAD_TSRMLS(this);
		
		if(include_http_globals){
			GET_HTTP_GLOBAL("_GET",    this->PG_http_globals, TRACK_VARS_GET);  
			GET_HTTP_GLOBAL("_POST",   this->PG_http_globals, TRACK_VARS_POST);       
			GET_HTTP_GLOBAL("_COOKIE", this->PG_http_globals, TRACK_VARS_COOKIE);     
			GET_HTTP_GLOBAL("_SERVER", this->PG_http_globals, TRACK_VARS_SERVER);     
			GET_HTTP_GLOBAL("_ENV",    this->PG_http_globals, TRACK_VARS_ENV);        
			GET_HTTP_GLOBAL("_FILES",  this->PG_http_globals, TRACK_VARS_FILES);      
			GET_HTTP_REQUEST_GLOBAL(   this->http_request_global); /*get $_REQUEST*/
		}

		/* save the current EG  */    
		this->EG_current_execute_data  =  EG(current_execute_data    ); 
		this->EG_argument_stack        =  EG(argument_stack          ); 
		this->EG_scope                 =  EG(scope                   ); 
		this->EG_This                  =  EG(This                    ); 
		this->EG_called_scope          =  EG(called_scope            ); 
		this->EG_active_symbol_table   =  EG(active_symbol_table     ); 
		this->EG_return_value_ptr_ptr  =  EG(return_value_ptr_ptr    ); 
		this->EG_active_op_array       =  EG(active_op_array         ); 
		this->EG_opline_ptr            =  EG(opline_ptr              ); 
		this->EG_error_zval            =  EG(error_zval              ); 
		this->EG_error_zval_ptr        =  EG(error_zval_ptr          ); 
		this->EG_user_error_handler    =  EG(user_error_handler      ); 
	}

	inline void SwapIn(bool include_http_globals){
		TSRMLS_FIELD;                            
		
		/* load EG from the task specific context*/                            
		PHPGO_LOAD_TSRMLS(this);                                      
		EG(current_execute_data )   =  this->EG_current_execute_data; 
		EG(argument_stack       )   =  this->EG_argument_stack      ; 
		EG(scope                )   =  this->EG_scope               ; 
		EG(This                 )   =  this->EG_This                ; 
		EG(called_scope         )   =  this->EG_called_scope        ; 
		EG(active_symbol_table  )   =  this->EG_active_symbol_table ; 
		EG(return_value_ptr_ptr )   =  this->EG_return_value_ptr_ptr; 
		EG(active_op_array      )   =  this->EG_active_op_array     ; 
		EG(opline_ptr           )   =  this->EG_opline_ptr          ; 
		EG(error_zval           )   =  this->EG_error_zval          ; 
		EG(error_zval_ptr       )   =  this->EG_error_zval_ptr      ; 
		EG(user_error_handler   )   =  this->EG_user_error_handler  ; 
		
		if(include_http_globals){
			memcpy( PG(http_globals), this->PG_http_globals, sizeof(PG(http_globals)) ); 	

			SET_HTTP_GLOBAL("_GET",   this->PG_http_globals, TRACK_VARS_GET);            
			SET_HTTP_GLOBAL("_POST",  this->PG_http_globals, TRACK_VARS_POST);           
			SET_HTTP_GLOBAL("_COOKIE",this->PG_http_globals, TRACK_VARS_COOKIE);         
			SET_HTTP_GLOBAL("_SERVER",this->PG_http_globals, TRACK_VARS_SERVER);         
			SET_HTTP_GLOBAL("_ENV",   this->PG_http_globals, TRACK_VARS_ENV);            
			SET_HTTP_GLOBAL("_FILES", this->PG_http_globals, TRACK_VARS_FILES);          
			SET_HTTP_REQUEST_GLOBAL(  this->http_request_global); /*set $_REQUEST*/     
		}
	}
};

struct PhpgoContext : public PhpgoBaseContext, public FreeableImpl{
public:
	PhpgoContext(uint64_t options TSRMLS_DC){
		TSRMLS_SET_CTX(this->TSRMLS_C);
		this->go_routine_options = options;
	}
};

// the Scheduler Context is essentially the same as the Task's context,
// but since Scheduler Context  is thread local and thread locals cannot
// have virtual members, we have to remove the FreeableImpl from the
// Scheduler Context...
struct PhpgoSchedulerContext : public PhpgoBaseContext{
public:
	PhpgoSchedulerContext(){
		TSRMLS_FETCH();                     // void ***tsrm_ls = (void ***) ts_resource_ex(0, NULL)
		TSRMLS_SET_CTX(this->TSRMLS_C);     // this->tsrm_ls = (void ***) tsrm_ls
	}
};

// the scheduler may be executed in multiple thread: 
// use thread local variable to store the scheduler EG's	
static thread_local PhpgoSchedulerContext scheduler_ctx;

/*null-out our concerned globals to avoid potential problem*/
#define PHPGO_INITIALIZE_RUNNING_ENVIRONMENT()                     \
{                                                                  \
	EG(current_execute_data )   =  NULL;                           \
	EG(argument_stack       )   =  NULL;                           \
	EG(scope                )   =  NULL;                           \
	EG(This                 )   =  NULL;                           \
	EG(called_scope         )   =  NULL;                           \
	EG(active_symbol_table  )   =  NULL;                           \
	EG(return_value_ptr_ptr )   =  NULL;                           \
	EG(active_op_array      )   =  NULL;                           \
	EG(opline_ptr           )   =  NULL;                           \
	INIT_ZVAL(EG(error_zval));                                     \
	EG(error_zval_ptr       )   =  NULL;                           \
	EG(user_error_handler   )   =  NULL;                           \
	/*do not do the following as we inherit globals from parent*/  \
	/*memset(PG(http_globals), 0, sizeof(PG(http_globals)) );*/    \
} 


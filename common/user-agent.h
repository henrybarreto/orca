/**
 * @file user-agent.h
 * @author cee-studio
 * @date 18 Jul 2021
 * @brief File containing internal functions and datatypes for HTTP Requests interfacing
 */

#ifndef USER_AGENT_H
#define USER_AGENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h> /* uint64_t */
#include <curl/curl.h> 
#include "ntl.h" /* struct sized_buffer */
#include "types.h" /* ORCAcode */
#include "logconf.h" /* logging facilities */

struct user_agent; /* forward declaration */

/*possible http methods */
enum http_method {
  HTTP_INVALID = -1,
  HTTP_DELETE, 
  HTTP_GET, 
  HTTP_POST, 
  HTTP_MIMEPOST, 
  HTTP_PATCH, 
  HTTP_PUT
};

/* COMMON HTTP RESPONSE CODES
https://en.wikipedia.org/wiki/List_of_HTTP_status_codes */
#define HTTP_OK                   200
#define HTTP_CREATED              201
#define HTTP_NO_CONTENT           204
#define HTTP_NOT_MODIFIED         304
#define HTTP_BAD_REQUEST          400
#define HTTP_UNAUTHORIZED         401
#define HTTP_FORBIDDEN            403
#define HTTP_NOT_FOUND            404
#define HTTP_METHOD_NOT_ALLOWED   405
#define HTTP_UNPROCESSABLE_ENTITY 422
#define HTTP_TOO_MANY_REQUESTS    429
#define HTTP_GATEWAY_UNAVAILABLE  502

#define UA_MAX_HEADER_SIZE 100 + 1

/*callback for object to be loaded by api response */
typedef void (load_obj_cb)(char *str, size_t len, void *p_obj);
typedef void (cxt_load_obj_cb)(void * cxt, char *str, size_t len, void *p_obj);

struct ua_resp_handle {
  void *cxt; /**< the context for cxt_ok_cb; */

  load_obj_cb *ok_cb;
  void *ok_obj; /**< the pointer to be passed to ok_cb */

  load_obj_cb *err_cb;
  void *err_obj; /**< the pointer to be passed to err_cb */

  cxt_load_obj_cb *cxt_ok_cb;  /**< ok callback with an execution context */
  cxt_load_obj_cb *cxt_err_cb; /**< err callback with an execution context */
};

struct ua_resp_header {
  char *buf;      /**< response header buffer */
  size_t length;  /**< response header string length */
  size_t bufsize; /**< real size occupied in memory by buffer */

  struct {  /**< array of header field/value pairs */
    struct {
      uintptr_t idx; /**< offset index of 'buf' for the start of field or value */
      size_t size;   /**< length of individual field or value */
    } field, value;
  } pairs[UA_MAX_HEADER_SIZE];
  int size; /**< number of elements initialized in `pairs` */
};

struct ua_resp_body {
  char *buf;      /**< response body buffer */
  size_t length;  /**< response body string length */
  size_t bufsize; /**< real size occupied in memory by buffer */
};

struct ua_info {
  struct loginfo loginfo;            /**< logging informational */
  int httpcode;                      /**< the HTTP response code */
  struct sized_buffer req_url;       /**< request URL */
  uint64_t req_tstamp;               /**< timestamp of when the request completed */
  struct ua_resp_header resp_header; /**< the response header */
  struct ua_resp_body resp_body;     /**< the response body */
};

const char* http_code_print(int httpcode);
const char* http_reason_print(int httpcode);
const char* http_method_print(enum http_method method);
enum http_method http_method_eval(char method[]);

void ua_reqheader_add(struct user_agent *ua, const char field[], const char value[]);
void ua_reqheader_del(struct user_agent *ua, const char field[]);
char* ua_reqheader_str(struct user_agent *ua, char *buf, size_t bufsize);

void ua_curl_easy_setopt(struct user_agent *ua, void *data, void (setopt_cb)(CURL *ehandle, void *data));
void ua_curl_mime_setopt(struct user_agent *ua, void *data, curl_mime* (mime_cb)(CURL *ehandle, void *data)); /* @todo this is temporary */

struct user_agent* ua_init(struct logconf *config);
struct user_agent* ua_clone(struct user_agent *orig_ua);
void ua_cleanup(struct user_agent *ua);

void ua_set_url(struct user_agent *ua, const char *base_url);
const char* ua_get_url(struct user_agent *ua);
void ua_block_ms(struct user_agent *ua, const uint64_t wait_ms);
ORCAcode ua_vrun(
  struct user_agent *ua,
  struct ua_info *info,
  struct ua_resp_handle *resp_handle,
  struct sized_buffer *req_body,
  enum http_method http_method, char endpoint[], va_list args);
ORCAcode ua_run(
  struct user_agent *ua,
  struct ua_info *info,
  struct ua_resp_handle *resp_handle,
  struct sized_buffer *req_body,
  enum http_method http_method, char endpoint[], ...);

void ua_info_cleanup(struct ua_info *info);
struct sized_buffer ua_info_respheader_field(struct ua_info *info, char field[]);
struct sized_buffer ua_info_get_resp_body(struct ua_info *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* USER_AGENT_H */

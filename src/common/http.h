#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <stdio.h>

struct http;

#define HTTP_VERSION "HTTP/1.0"

enum http_status {
	HTTP_OK						= 200,
	HTTP_BAD_REQUEST			= 400,
	HTTP_NOT_FOUND				= 404,
	HTTP_INTERNAL_SERVER_ERROR	= 500,
};

/*
 * Create a new HTTP connect using the given socket.
 */
int http_create (struct http **httpp, int sock);

/*
 * Send a HTTP request line.
 */
int http_write_request (struct http *http, const char *method, const char *fmt, ...)
	__attribute((format (printf, 3, 4)));

/*
 * Send a HTTP response line.
 *
 * Reason can be passed as NULL if status is a recognized status code.
 */
int http_write_response (struct http *http, enum http_status status, const char *reason);

/*
 * Send one HTTP header.
 */
int http_write_headerv (struct http *http, const char *header, const char *fmt, va_list args);
int http_write_header (struct http *http, const char *header, const char *fmt, ...)
	__attribute((format (printf, 3, 4)));

/*
 * End the HTTP headers.
 */
int http_write_headers (struct http *http);

/*
 * Send a HTTP request body from a FILE.
 *
 * Returns 1 on EOF, <0 on error.
 */
int http_write_file (struct http *http, FILE *file, size_t content_length);




/*
 * Read a HTTP request.
 */
int http_read_request (struct http *http, const char **methodp, const char **pathp, const char **versionp);

/*
 * Read a HTTP response.
 */
int http_read_response (struct http *http, const char **versionp, unsigned *statusp, const char **reasonp);

/*
 * Read next header as {*headerp}: {*valuep}.
 *
 * In case of a folded header, *headerp is left as-is.
 *
 * Returns 1 on end-of-headers, 0 on header, <0 on error.
 */
int http_read_header (struct http *http, const char **headerp, const char **valuep);

/*
 * Read (part of) the response body.
 *
 * The size of the given buffer is passed in *lenp, and the number of bytes read in returned in *lenp.
 *
 * Returns 1 on EOF, <0 on error.
 */
int http_read_raw (struct http *http, char *buf, size_t *lenp);

/*
 * Read the response body into FILE, or discard if NULL.
 */
int http_read_file (struct http *http, FILE *file, size_t content_length);





/*
 * Release all associated resources.
 */
void http_destroy (struct http *http);

#endif

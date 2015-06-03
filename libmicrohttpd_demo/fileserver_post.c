#include "fileserver_example.h"

#define POSTBUFFERSIZE  512
#define MAXNAMESIZE     20
#define MAXANSWERSIZE   512
#define GET             0
#define POST            1
struct connection_info_struct
{
	int connectiontype;
	char *answerstring;
	struct MHD_PostProcessor *postprocessor;
};

static int
iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
const char *filename, const char *content_type,
const char *transfer_encoding, const char *data, uint64_t off,
size_t size)
{
	struct connection_info_struct *con_info = coninfo_cls;

	if (0 == strcmp(key, "name"))
	{
		if ((size > 0) && (size <= MAXNAMESIZE))
		{
			char *answerstring;
			answerstring = malloc(MAXANSWERSIZE);
			if (!answerstring)
				return MHD_NO;
			const char *greetingpage =
				"Welcome, %s!";
			snprintf(answerstring, MAXANSWERSIZE, greetingpage, data);
			con_info->answerstring = answerstring;
		}
		else
			con_info->answerstring = NULL;

		return MHD_NO;
	}

	return MHD_YES;
}

static ssize_t
file_reader (void *cls, uint64_t pos, char *buf, size_t max)
{
  FILE *file = cls;

  (void)  fseek (file, pos, SEEK_SET);
  return fread (buf, 1, max, file);
}

static void
free_callback (void *cls)
{
  FILE *file = cls;
  fclose (file);
}

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data,
	  size_t *upload_data_size, void **ptr)
{

  static int aptr;
  struct MHD_Response *response;
  int ret;
  FILE *file;
  struct stat buf;

  if (NULL == *ptr)
  {
	  struct connection_info_struct *con_info;

	  con_info = malloc(sizeof (struct connection_info_struct));
	  if (NULL == con_info)
		  return MHD_NO;
	  con_info->answerstring = NULL;

	  if (0 == strcmp(method, "POST"))
	  {
		  con_info->postprocessor =
			  MHD_create_post_processor(connection, POSTBUFFERSIZE,
			  iterate_post, (void *)con_info);

		  if (NULL == con_info->postprocessor)
		  {
			  free(con_info);
			  return MHD_NO;
		  }

		  con_info->connectiontype = POST;
	  }
	  else
		  con_info->connectiontype = GET;

	  *ptr = (void *)con_info;

	  return MHD_YES;
  }

  if (0 == strcmp(method, MHD_HTTP_METHOD_GET))
  {
	  if (0 == stat(&url[1], &buf))
		  file = fopen(&url[1], "rb");
	  else
		  file = NULL;
	  if (file == NULL)
	  {
		  response = MHD_create_response_from_buffer(strlen(PAGE),
			  (void *)PAGE,
			  MHD_RESPMEM_PERSISTENT);
		  ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
		  MHD_destroy_response(response);
	  }
	  else
	  {
		  response = MHD_create_response_from_callback(buf.st_size, 32 * 1024,     /* 32k page size */
			  &file_reader,
			  file,
			  &free_callback);
		  if (response == NULL)
		  {
			  fclose(file);
			  return MHD_NO;
		  }
		  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		  MHD_destroy_response(response);
	  }
	  return ret;
  }
  
  if (0 == strcmp(method, "POST"))
  {
	  struct connection_info_struct *con_info = *ptr;

	  if (*upload_data_size != 0)
	  {
		  MHD_post_process(con_info->postprocessor, upload_data,
			  *upload_data_size);
		  *upload_data_size = 0;

		  return MHD_YES;
	  }
	  else if (NULL != con_info->answerstring)
	  {
		  int ret;
		  struct MHD_Response *response;


		  response =
			  MHD_create_response_from_buffer(strlen(con_info->answerstring), (void *)(con_info->answerstring),
			  MHD_RESPMEM_PERSISTENT);
		  if (!response)
			  return MHD_NO;

		  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		  MHD_destroy_response(response);

		  return ret;
	  }
	
  }
}

int fileserver(int port)
{
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
                        port,
                        NULL, NULL, &ahc_echo, PAGE, MHD_OPTION_END);
  while(1);
}

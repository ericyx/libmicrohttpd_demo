#include <fileserver_example.h>

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

  if (0 != strcmp (method, MHD_HTTP_METHOD_GET))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }
  *ptr = NULL;                  /* reset when done */
  if (0 == stat (&url[1], &buf))
    file = fopen (&url[1], "rb");
  else
    file = NULL;
  if (file == NULL)
    {
      response = MHD_create_response_from_buffer (strlen (PAGE),
						  (void *) PAGE,
						  MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
      MHD_destroy_response (response);
    }
  else
    {
      response = MHD_create_response_from_callback (buf.st_size, 32 * 1024,     /* 32k page size */
                                                    &file_reader,
                                                    file,
                                                    &free_callback);
      if (response == NULL)
	{
	  fclose (file);
	  return MHD_NO;
	}
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }
  return ret;
}

int fileserver(int port)
{
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
                        port,
                        NULL, NULL, &ahc_echo, PAGE, MHD_OPTION_END);
  while(1);
}

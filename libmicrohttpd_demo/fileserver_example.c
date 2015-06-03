//demo curl -d 'json={"instances":2,"profile":"OSD"}' localhost:7777

#include "fileserver_example.h"
#include <cJSON.h>
#define POSTBUFFERSIZE  1512
#define MAXNAMESIZE     1512
#define MAXANSWERSIZE   1512
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

	if (1/*0 == strcmp(key, "json")*/)
	{
		if ((size > 0) && (size <= MAXNAMESIZE))
		{
			char *answerstring;
			answerstring = malloc(MAXANSWERSIZE);
			if (!answerstring)
				return MHD_NO;

			/***********************
			int value_int;
			char *value_string;
			cJSON *json;
			json = cJSON_Parse(key);
			value_int = cJSON_GetObjectItem(json, "instances")->valueint;
			value_string = cJSON_GetObjectItem(json, "profile")->valuestring;
			*/
			const char *greetingpage =
				"Welcome, %s,%s!\n";
			//"Welcome, %s,%s!\n";

			snprintf(answerstring, MAXANSWERSIZE, greetingpage, key, data);
			//snprintf(answerstring, MAXANSWERSIZE, greetingpage, key, data);
			con_info->answerstring = answerstring;
		}
		else
			con_info->answerstring = NULL;

		return MHD_NO;
	}

	return MHD_YES;
}

static ssize_t
file_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	FILE *file = cls;

	(void)fseek(file, pos, SEEK_SET);
	return fread(buf, 1, max, file);
}

static void
free_callback(void *cls)
{
	FILE *file = cls;
	fclose(file);
}

static int
ahc_echo(void *cls,
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
		ConnectionData* connection_data = NULL;

		connection_data = static_cast<ConnectionData*>(*con_cls);
		if (NULL == connection_data)
		{
			connection_data = new ConnectionData();
			connection_data->is_parsing = false;
			*con_cls = connection_data;
		}

		if (!connection_data->is_parsing)
		{
			// First this method gets called with *upload_data_size == 0
			// just to let us know that we need to start receiving POST data
			connection_data->is_parsing = true;
			return MHD_YES;
		}
		else
		{
			if (*upload_data_size != 0)
			{
				// Receive the post data and write them into the bufffer
				connection_data->read_post_data << string(upload_data, *upload_data_size);
				*upload_data_size = 0;
				return MHD_YES;
			}
			else
			{
				// *upload_data_size == 0 so all data have been received
				output = "Received data:\n\n";
				output += connection_data->read_post_data.str();
				output += "\n";
				delete connection_data;
				connection_data = NULL;
				*con_cls = NULL;
			}
		}

		const char* output_const = output.c_str();

		struct MHD_Response *response = MHD_create_response_from_buffer(
			strlen(output_const), (void*)output_const, MHD_RESPMEM_MUST_COPY);

		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, content_type.c_str());

		int ret = MHD_queue_response(connection, http_code, response);

		MHD_destroy_response(response);

		return ret;

	}
}

int fileserver(int port)
{
	struct MHD_Daemon *d;

	d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_DEBUG,
		port,
		NULL, NULL, &ahc_echo, PAGE, MHD_OPTION_END);
	while (1);
}

#include <string>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cstring>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
extern "C"
{
#include <microhttpd.h>
#include <platform.h>
#include <unistd.h>
}
#define MAXANSWERSIZE   1512
using namespace std;
using namespace rapidjson;
#define PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"

static ssize_t
file_reader(void *cls, uint64_t pos, char *buf, size_t max)
{
	FILE *file = (FILE*)cls;

	(void)fseek(file, pos, SEEK_SET);
	return fread(buf, 1, max, file);
}

static void
free_callback(void *cls)
{
	FILE *file = (FILE*)cls;
	fclose(file);
}


struct ConnectionData
{
	bool is_parsing;
	stringstream read_post_data;
};


int handle_request(void *cls, struct MHD_Connection *connection,
	const char *url,
	const char *method, const char *version,
	const char *upload_data,
	size_t *upload_data_size, void **con_cls)
{
	struct MHD_Response *response;
	int ret;


	if (strcmp(method, MHD_HTTP_METHOD_POST) == 0)
	{
		string output;
		string content_type = "text/plain";
		const char* json;
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
				string outpost = connection_data->read_post_data.str();
				json = outpost.c_str();
				delete connection_data;
				connection_data = NULL;
				*con_cls = NULL;
			}
		}
		//const char* output_const = output.c_str();

		//const char* output_const = postdata.c_str();
		Document d;
		d.Parse(json);

		string out = d["profile"].GetString();
		const char* output_const = out.c_str();
		response = MHD_create_response_from_buffer(
			strlen(output_const), (void*)output_const, MHD_RESPMEM_MUST_COPY);

		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, content_type.c_str());

		ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

		MHD_destroy_response(response);

		return ret;
	}
	else if (0 == strcmp(method, MHD_HTTP_METHOD_GET))
	{
		static int aptr;
		FILE *file;
		struct stat buf;
		if (&aptr != *con_cls)
		{
			/* do never respond on first call */
			*con_cls = &aptr;
			return MHD_YES;
		}
		*con_cls = NULL;                  /* reset when done */
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


}

int main(int argc, char** argv)
{

	MHD_Daemon* daemon = MHD_start_daemon(
		MHD_USE_SELECT_INTERNALLY, 8001, NULL, NULL,
		&handle_request, NULL, MHD_OPTION_END);

	if (!daemon) {
		cerr << "Failed to start HTTP server " << endl;
		return 1;
	}
	while (1);
	// Handle requests until we're asked to stop
	/*for (;;) {
	MHD_run(daemon);
	usleep(1000 * 1000);
	}*/
	return 0;
}

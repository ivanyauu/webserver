# Contributor Documentation

### Team B2-03: new-grad-ten-years-experience
### Contributors: Michael Yang, Justin Nerhood, Eric Gan, Ivan Yau

## Table of contents

1. [Source Code Layout](#source-code-layout)
2. [Build, Test, Run](#build,-test,-run)

    a. [Build](#build)
    
    b. [Test](#test)

    c. [Run](#run)

3. [Request Handlers](#request-handlers)

    a. [Router](#router)

    b. [Request Handler Class](#request-handler-class)

    c. [Static Handler](#static-handler)

    d. [Echo Handler](#echo-handler)

    e. [404 Handler](#404-handler)

    f. [CRUD Handler](#crud-handler)

    g. [Sleep Handler](#sleep-handler)

    h. [Health Handler](#health-handler)

    i. [Markdown Handler](#markdown-handler)

    j. [New Request Handler](#new-request-handler)

## Source code layout

### Top Level Directories
_```/cmake```_
- The top level ```/cmake``` directory contains the config for our coverage report. This is the only file within this directory.

_```/configs```_
- The top level ```/configs``` directory contains the necessary config files for both running and testing the server. Specifically, it has two config files. One, ```deploy.conf```, is used as the config passed in whenever the server runs, and the other, ```integration.conf```, is used as the config for the integration tests.

_```/docker```_
- The top level ```/docker``` directory contains all of the necessary information for running our docker environment and the setup for Google cloud.

_```/include```_
- The top level ```/include``` directory contains all of the header classes for the various classes that we use throughout the source code. 
Specifically, the files are
   1. ```config_parser.h```
This file deals with parsing the needed things from given config files.
   2. ```echo_handler.h```
This defines our ```echo_handler``` class, used for the echo functionality of the server.
   3. ```logger.h```
This defines our logger class which logs all the necessary information as the server runs.
   4. ```notfound_handler.h```
This defines our notfound handler, which deals with serving 404s for not found files.
   5. ```request_handler.h```
This file defines the abstract base class from which all of our handlers inherit from.
   6. ```router.h```
This file defines our router class, which handles using the necessary request handlers at appropriate times.
   7. ```server.h```
Here we define our actual server object, which is used for connecting to clients.
   8. ```session.h```
The session class defined here deals with writing and reading the actual messages in a given connection.
   9. ```static_handler.h```
This contains another definition of a handler class, the ```static_handler```. This handler serves static files on appropriate routes.
   10. ```crud_handler.h```
This contains another definition of a handler class, the ```crud_handler```. This handler supports POST, GET, PUT, and DELETE requests pertinent to CRUD operations on the filesystem.

_```/src```_
- This top level ```/src``` directory contains all of the actual implementation files for the header files defined in the ```/include``` directory described above.

_```/static```_
- The top level ```/static``` directory contains static files to be served during the testing of the server. These are primarily dummy files whose main purpose is to validate tests.

_```/tests```_
- The top level ```/tests``` directory contains all of the testing necessary for our server. This includes both unit tests and integration tests. This directory contains one subdirectory, ```/tests/example_configs```, which contains example config files to be passed in for testing the config_parser.

## Build, Test, Run
### Build
Our code uses CMake to generate Makefiles for building. Builds should be performed out of source. Make sure that the CMakeLists.txt file is updated with the proper build configuration, then run
```
git submodule add -f https://github.com/commonmark/cmark.git external/cmark
mkdir build
cd build
cmake ..
make
```
Note `git submodule` is used to initialize and manage [cmark](https://github.com/commonmark/cmark), an external dependency for parsing Markdown to HTML

### Test
Assuming that the build has been completed as specified above, unit and integration tests can be run with the command
```
ctest
```
The `-V` command can be added for verbose logging, or `--output-on-failure` to log for only failed tests.

We use gcovr to measure test coverage. To get test coverage results, start in the root directory and run the following
```
mkdir build_coverage
cd build_coverage
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```


### Run
The server can be run either locally or in a docker container. To run locally, first run the build steps above. Then the command
```
bin/server ../configs/deploy.conf
```
will bring up the server on port 80.

To run the server in docker, start in the root directory. First build the base image:
```
docker build -f docker/base.Dockerfile -t new-grad-ten-years-experience:base .
```
Then build the server image:
```
docker build -f docker/Dockerfile -t server_image .
```

Now run the docker container using
```
docker run --rm  -p 80:80 --name my_run server_image:latest
```
The `-p` flag port forwards the local port 80 to port 80 on the container, which the server is listening on. You should be able to see the running container with `docker ps`.

## Request Handlers
### Router
Our current request handler first parses the config files and gets the handlers with this code 
```cpp
std::vector<HandlerConfig> handlers = config.GetRequestHandlers();
```
Once we receive the handlers we then match the handlers with our handler_registry which is in our **router.cc** file, which contains pre-defined handlers.
```cpp
std::unordered_map<std::string, request_handler_factory> router::handler_registry_ = {
  {"echo_handler", echo_handler::init},
  {"static_handler", static_handler::init},
  {"notfound_handler", notfound_handler::init},
  {"crud_handler", crud_handler::init}
};
```
Using the handler registry, we determine if the handler requested is in our registry using a for loop that checks for the longest matching prefix. This determines the handler we will be using. We have named this function match cause it matches the requested handler with a predefined one.
```cpp
std::unique_ptr<request_handler> router::match(const std::string& path) {
  Logger *logger = Logger::get_global_log();

  // Look for a handler with longest matching prefix
  int longest_match = 0;
  std::string handler_name = "";
  std::string root = "";
  for (const auto& handler: handlers_) {
    if (path.find(handler.path) == 0 && handler.path.size() > longest_match) {
      longest_match = handler.path.size();
      handler_name = handler.name;
      root = handler.root;
    }
  }

  logger->logDebug("Received handler " + handler_name);
  request_handler_factory factory = router::handler_registry_[handler_name];
  if (!factory) {
    return nullptr;
  }
  return factory(root);
}
```

### Request Handler Class
As required in the common API, we created a request handler interface. This interface is used by all existing and newly created request handlers, as all request handlers extend this class. You can find this file in **/include/request_handler.h**

```cpp
// Base class defining an HTTP request handler.
#include <string>
#include <functional>
#include <boost/beast/http.hpp>
namespace http = boost::beast::http;

class request_handler {
public:
    // Takes an HTTP request and returns and HTTP response.
    virtual http::response<http::string_body> handle_request(http::request<http::string_body> request) = 0;
};

// Function that creates a request handler given a root string.
using request_handler_factory = std::function<std::unique_ptr<request_handler>(std::string)>;
```

Using this interface, we have flexibility in creating new request handlers. For example in the **static_handler.cc** file, you can see that we can create higher abstraction by creating a unique pointer below:

```cpp
std::unique_ptr<request_handler> static_handler::init(std::string root) {
    return std::make_unique<static_handler>(root);
}
```

Then in the **session.cc** file you can see that we don't specify the type of handler to process, but just a handler

```cpp
std::string session::process_request(const http::request<http::string_body>& parsed) {
  logger->logDebug("Processing the Request");

  http::response<http::string_body> response;
  response.version(11);

  http::request_parser<http::string_body> parser;
  boost::beast::error_code ec;
  parser.put(boost::asio::buffer(request), ec);
  if (ec) {
    response.result(http::status::bad_request);
    return boost::lexical_cast<std::string>(response);
  }

  http::request<http::string_body> parsed = parser.release();
  std::string path = std::string(parsed.target());

  std::unique_ptr<request_handler> handler = router_.match(path);
  if (handler == nullptr) {
    response.result(http::status::internal_server_error);
    return boost::lexical_cast<std::string>(response);
  }

  return boost::lexical_cast<std::string>(handler->handle_request(parsed));
}
```

### Static Handler
Our current static handler follows the Common API voted in class. The handle_request function receives a ```boost::beast::http::request``` object and returns a ```boost::beast::http::response``` object. First we check if the file is found, then we determine the content type of the request, and then it loads the file into a payload. If the file is not found we return a 404 not found error. This is all within our handle_request function.

For markdown files, the static handler additionally supports the ability to return the file as raw or parsed into HTML by adding a parameter `?raw=true` or `?raw=false` to the end of the URL. By default, the handler will parse markdown files to HTML.

You can find this function in **/src/static_handler.cc**

```cpp
std::unique_ptr<request_handler> static_handler::init(std::string root) {
    return std::make_unique<static_handler>(root);
}

static_handler::static_handler(std::string root): root_(root) {}

http::response<http::string_body> static_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);

    std::string filepath = std::string(request.target());

    // Get parameters.
    std::string parameter = "";
    if(filepath.find_last_of("?") != std::string::npos) {
        parameter = filepath.substr(filepath.find_last_of("?") + 1);
        filepath = filepath.substr(0, filepath.find_last_of("?"));
    }

    // File found.
    if(std::filesystem::exists(root_ + filepath) && !std::filesystem::is_directory(root_ + filepath)) {
        response.result(http::status::ok);

        // Determine content type.
        std::string file_extension = "";
        std::string filename = filepath.substr(filepath.find_last_of("/") + 1);
        if(filename.find_last_of(".") != std::string::npos) {
            file_extension = filename.substr(filename.find_last_of(".") + 1);
        }
        if(file_extension == "html" || file_extension == "md") {
            response.set(http::field::content_type, "text/html");
        } else if(file_extension == "jpg" || file_extension == "jpeg") {
            response.set(http::field::content_type, "image/jpeg");
        } else if(file_extension == "txt") {
            response.set(http::field::content_type, "text/plain");
        } else if(file_extension == "zip") {
            response.set(http::field::content_type, "application/zip");
        } else {
            response.set(http::field::content_type, "application/octet-stream");
        }

        // Load file into payload.
        std::FILE* file = std::fopen((root_ + filepath).c_str(), "r");
        std::stringstream payload;
        int current = std::fgetc(file);
        while(current != EOF) {
            payload.put(current);
            current = std::fgetc(file);
        }
        std::fclose(file);

        // Process file if necessary.
        if(file_extension == "md") {
            MarkdownToHtml parser;
            if(parameter == "" || parameter == "raw=false") {
                response.body() = parser.convert(payload.str());
            } else if(parameter == "raw=true") {
                response.body() = payload.str();
            } else {
                response.set(http::field::content_type, "text/plain");
                response.result(http::status::bad_request);
                response.body() = "Bad request";
            }
        } else {
            response.body() = payload.str();
        }

    // File not found.
    } else {
        response.result(http::status::not_found);
        response.body() = "File not found";
    }

    response.prepare_payload();
    return response;
}
```

### Echo Handler
Below is our implementation of the echo handler, which checks if the the request is an echo request. Then it basically echos the response to the client, as we set the response.body() equal to the request.

```cpp
namespace http = boost::beast::http;

std::unique_ptr<request_handler> echo_handler::init(std::string root) {
    return std::make_unique<echo_handler>();
}

echo_handler::echo_handler() {}

http::response<http::string_body> echo_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);
    response.set(http::field::content_type, "text/plain");
    response.body() = boost::lexical_cast<std::string>(request);
    response.prepare_payload();
    return response;
}
```

### 404 Handler
If we receive a request with only /, then we want to return a 404 not found. Our implementation returns an error in html form.
```cpp
std::unique_ptr<request_handler> notfound_handler::init(std::string root) {
    return std::make_unique<notfound_handler>();
}

notfound_handler::notfound_handler() {}

http::response<http::string_body> notfound_handler::handle_request(http::request<http::string_body> request) {
    http::response<http::string_body> response;
    response.version(11);
    response.set(http::field::content_type, "text/html");
    response.result(http::status::not_found);
    response.body() = "<html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>";
    response.prepare_payload();
    return response;
}
```

### CRUD Handler
Requires a root directory and file_io pointer for construction.\
It delegates to ```handle_post_request```, ```handle_get_request```, ```handle_put_request```, and ```handle_delete_request``` if the HTTP method is POST, GET, PUT, or DELETE, respectively.  *Note: an entity is another name for an object.*
- If HTTP method is not POST/GET/PUT/DELETE, send 405 Method Not Allowed.

##### Create (POST)
Allow upload of JSON data without any ID with an HTTP POST of JSON data (in the POST body), returning the ID of the newly created object as JSON. Thread-safe ID generation is accomplished through Boost UUID generation.\
- Upon file creation completion, send 201 Created with JSON ```{"id": "<id>"}``` as response body.
- If request has no body, send 400 Bad Request with "text/plain" body as "No data in POST request".
- If request URI format is not \<crud-prefix\>/\<entity-dir\>, send 400 Bad Request with "text/plain" body as "No entity directory specified".
- If request is not sending JSON data, send 415 Unsupported Media Type with "text/plain" body as "Content-Type must be application/json".
- If any file I/O operation fails, send 500 Internal Server Error with stock 500 "text/html" body.

##### Read (GET)
Allow retrieval of JSON data for a given ID with an HTTP GET with the ID in the request URL. If instead an entity directory is provided, allow retrieval of existing IDs within an Entity with an HTTP GET with the Entity type in the request URL (and no ID).
- Upon data retrieval completion, send 200 OK with the file's JSON data as response body.
- Upon ids retrieval completion, send 200 OK with response body in JSON array format: e.g. ```["<id1>","<id2>", ...]```. Note that no ids means the body is JSON ```[]```.
- If request URI format is not \<crud-prefix\>/\<entity-dir\>/\<id\> or \<crud-prefix>/\<entity-dir\>, send 400 Bad Request with stock 400 "text/html" body.
- If directory does not exist for ids retrieval, send 400 Bad Request with stock 400 "text/html" body.
- If file does not exist for data retrieval, send 404 Not Found with stock 404 "text/html" body.
- If any file I/O operation fails, send 500 Internal Server Error with stock 500 "text/html" body.

##### Update (PUT)
Allow upload of JSON data to a specific ID with an HTTP PUT of JSON data (in the PUT body), ie. file update. If the file does not exist, the behavior is the same as Create (POST), ie. file creation.
- Upon file update completion, send 204 No Content.
- Upon file creation completion, send 201 Created with ```{"id": "<id>"}``` as JSON body.
- If request has no body, send 400 Bad Request with "text/plain" body as "No data in PUT request".
- If request URI format is not \<crud-prefix\>/\<entity-dir\>/\<id\>, send 400 Bad Request with "text/plain" body as "File format must be ```<crud-prefix>/<entity-dir>/<id>```".
- If request is not sending JSON data, send 415 Unsupported Media Type with "text/plain" body as "Content-Type must be application/json".
- If any file I/O operation fails, send 500 Internal Server Error with "text/plain" body as "Unable to create or update file. Please try again later.".

##### Delete (DELETE)
Allow deletion of stored data for a specific ID with an HTTP DELETE with the ID in the request URL.
- Upon file deletion completion, send 204 No Content.
- If request URI format is not \<crud-prefix\>/\<entity-dir\>/\<id\>, send 400 Bad Request with "text/plain" body as "File format must be ```<crud-prefix>/<entity-dir>/<id>```".
- If file does not exist, treat it as file deletion, and send 204 No Content.

### Sleep Handler

The sleep handler blocks for one second before returning a 200 OK response; this is used to test multithreading.

### Health Handler

The health handler is invoked to gauge the health of the server, simply returning a 200 OK response with payload `OK`; if this response is received, it can be reasoned that the server is healthy.

### Markdown Handler
The markdown handler follows the same API pattern as the CRUD handler, except the markdown handler accepts markdown files (Content-Type=text/markdown). The markdown handler listens on paths with matching the `<markdown-prefix>`, which by default is `/markdown`. When retrieving a markdown object, the markdown file is converted into equivalent HTML so that it can be displayed in the browser. To retrive the raw markdown file, the url parameter `raw=true` can be added to the request, e.g.
```
<ip-address>/<markdown-prefix>/<entity>/<id>?raw=true
```

### New Request Handler
To create a new request handler, as mentioned before, we have created greater abstraction with the unique pointers and we have also implemented request_handler_factory, which is a function that creates a request handler given a root string.

```cpp
using request_handler_factory = std::function<std::unique_ptr<request_handler>(std::string)>;
```

Then we want to add the request handler into our handler_registry
```cpp
std::unordered_map<std::string, request_handler_factory> router::handler_registry_ = {
  {"echo_handler", echo_handler::init},
  {"static_handler", static_handler::init},
  {"notfound_handler", notfound_handler::init},
  {"crud_handler", crud_handler::init},
  {"sleep_handler", sleep_handler::init},
  {"health_handler", health_handler::init},
  {"markdown_handler", markdown_handler::init},
};
```

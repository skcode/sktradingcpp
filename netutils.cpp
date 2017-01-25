#include "logger.hpp"
#include <iostream>
#include <string>
#include <curl/curl.h> //your directory may be different
#include <cstdlib>
#include <cstdio>
#include <cstring>
using namespace std;


namespace netutils
{


 struct MemoryStruct
 {
     char *memory;
     size_t size;
 };


size_t
    WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        size_t realsize = size * nmemb;
        struct MemoryStruct *mem = (struct MemoryStruct *)userp;

        mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
        if(mem->memory == NULL)
        {
            /* out of memory! */
            LOG_ERROR("not enough memory (realloc returned NULL)");
            return 0;
        }

        memcpy(&(mem->memory[mem->size]), contents, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;

        return realsize;
    }

const string httpfetch(string url)
    {
        CURL *curl_handle;
        CURLcode res;

        struct MemoryStruct chunk;

        chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */

        curl_global_init(CURL_GLOBAL_ALL);

        /* init the curl session */
        curl_handle = curl_easy_init();

        /* specify URL to get */
        curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

        /* complete connection within 30 seconds */
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 30L);

        /* complete within 30 seconds */
        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);

        /* send all data to this function  */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        /* some servers don't like requests that are made without a user-agent
           field, so we provide one */
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        /* get it! */
        res = curl_easy_perform(curl_handle);

        /* check for errors */
        if(res != CURLE_OK)
        {
            THROW_EXCEPTION("curl_easy_perform() failed: "<<curl_easy_strerror(res));
            //fprintf(stderr, "curl_easy_perform() failed: %s\n",
             //       curl_easy_strerror(res));
        }


        /* cleanup curl stuff */
        curl_easy_cleanup(curl_handle);
        string ret;
        ret.append(chunk.memory,(long)chunk.size);
        if(chunk.memory)
            free(chunk.memory);

        /* we're done with libcurl, so clean it up */
        curl_global_cleanup();

        return ret;
    }

};

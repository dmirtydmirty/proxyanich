#include <curl/curl.h>
#include <cstdio>

struct speed_test{
    
    int init(){
        m_curl = curl_easy_init();
        if(not m_curl) {
            fprintf(stderr, "curl not init");
            return -1;
        }
        return 0;
    }
    CURL* m_curl;
};


int main()
{

}
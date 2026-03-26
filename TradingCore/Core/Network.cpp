#include "Network.h"

size_t Network::HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata)
{
    size_t total = size * nitems;

    if (userdata != nullptr)
    {
        std::string* headerBuf = static_cast<std::string*>(userdata);
        headerBuf->append(buffer, total);
    }

    return total;
}

size_t Network::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    userp->append((char*)contents, size * nmemb);

    return size * nmemb;
}
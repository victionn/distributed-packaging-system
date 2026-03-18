#define MIN_PAYLOAD 1024
#define MAX_PAYLOAD 65535
#define MIN_PEERS 1
#define MAX_PEERS 2048

//Used to store information on the config
struct config_obj {
    char* directory;
    int max_peers;
    int port;
};

//Loads in a config object parsed from a config file
struct config_obj* config_load(const char* path);
//Verify the config objects
void verifyConfigContents(struct config_obj* obj);
//Free the config object
void freeConfigObj(struct config_obj* obj);
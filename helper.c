/* helper file for proxy.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef struct cache {
	struct cache *next;
	unsigned int length;
	char *URL;
	char *content;
} cache_t;

#define MAX_MSG_LENGTH (1024*16)
#define MAX_BACK_LOG (5)

cache_t *cache;
int cache_size;


//implement this for getting/processing getaddrinfo stuff; fills out ip_addr field
int name_to_ipaddr(char *URL, char *servname, char *ip_addr);

//add entry to end of cache, in accordance with LRU
int add_cache_entry(char *data, char *URL);

//moves entry used in the middle to the end of the cache, has become most recently used
int enforce_LRU_middle(char *URL);

//removes first entry (LRU entry) in cache
int enforce_LRU_head();


int name_to_ipaddr(char *URL, char *servname, char *ip_addr) {
	int sockfd, rv;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; //force IPv6
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(URL, servname, &hints, &servinfo)) != 0) {
		printf("Error with getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	//loop through all results, connect with first one possible
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("Socket: ");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Connect: ");
			continue;
		}
		info = (struct sockaddr_in *)p->ai_addr;
		memcpy(ip_addr, inet_ntoa(info->sin_addr), sizeof(inet_ntoa(info->sin_addr)));
		break; //must have connected successfuly if reach this point
	}
	if (p == NULL) {
		//got to end of the list, made no connections
		printf("Failed to connect in getaddrinfo\n");
		return 1;
	}
	freeaddrinfo(servinfo); //finished with this struct
	return 0;
}

int add_cache_entry(char *data, char *URL) {
	cache_t *obj = (cache_t *)malloc(sizeof(cache_t));
	obj->content = data;
	obj->URL = URL;
	obj->next = NULL;
	obj->length = (int) (sizeof(cache_t) + sizeof(data));

	cache_t *iterater = cache;
	while (iterater->next != NULL) {
		iterater = iterater->next;
	}
	iterater->next = obj;
	return 0;
}

int enforce_LRU_middle(char *URL) {
	cache_t *iterator = cache;
	cache_t *prev = iterator;
	cache_t *last = cache;
	while (last->next != NULL) {
		last = last->next;
	}
	while (iterator != NULL) {
		if (strcmp(iterator->URL, URL) == 0) { //FOUND IT
			if ((iterator == cache && iterator->next == NULL)
				|| (iterator != cache && iterator->next == NULL)) {
				//first and only entry OR last entry in cache of size > 1
				return 0; //do nothing, just leave it
			} else if (iterator == cache && iterator->next != NULL) {
				//first entry in cache of size > 1
				last->next = iterator;
				cache->next = iterator->next->next;
				iterator->next = NULL;
				last = last->next;
				return 0;
			} else if (iterator != cache && iterator->next != NULL) {
				//somewhere in the middle of a cache of size > 1
				prev->next = iterator->next;
				last->next = iterator;
				iterator->next = NULL;
				last = iterator;
				return 0;
			}
		}
		prev = iterator;
		iterator = iterator->next;
	}
	return 1;
}

int enforce_LRU_head() {
	if (cache->next == NULL) {
		cache->length = 0;
		cache->URL = "";
		cache->content = "";
	}
	else {
		cache = cache->next;
	}
	return 0;
}


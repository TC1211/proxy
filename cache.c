/* helper file for proxy.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef struct cache {
	struct cache *next;
	unsigned int length; //should be in B?
	char *URL;
	char *content;
} cache_t;


#define MAX_MSG_LENGTH (1024*16)
#define MAX_BACK_LOG (5)
#define MAX_HOST_LENGTH (255)
#define IP_ADDR_LENGTH (32)

cache_t *cache;
int cache_size/*in B*/, max_cache_size/*in B*/;

//add entry to end of cache, in accordance with LRU
int add_cache_entry(char *data, char *URL);

//moves entry used in the middle to the end of the cache, has become most recently used
int enforce_LRU_middle(char *URL);

//removes first entry (LRU entry) in cache
int enforce_LRU_head();

//search cache by URL
cache_t *search_cache(char *URL);


int add_cache_entry(char *data, char *URL) {
	//MUST CHECK AVAILABLE CACHE SPACE!
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
	cache_size -= cache->length;
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

cache_t *search_cache(char *URL) {
	cache_t *iterator = cache;
	while (iterator != NULL) {
		if (strcmp(iterator->URL, URL) == 0) {
			return iterator;
		}
		iterator = iterator->next;
	}
	return NULL;
}

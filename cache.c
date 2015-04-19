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
#define MAX_HOST_LENGTH (255)
#define IP_ADDR_LENGTH (32)

//add entry to end of cache, in accordance with LRU
int add_cache_entry(cache_t **, char *, char *);

//moves entry used in the middle to the end of the cache, has become most recently used
int enforce_LRU_middle(cache_t **, char *);

//removes first entry (LRU entry) in cache
int enforce_LRU_head();

//search cache by URL
cache_t *search_cache(cache_t **, char *);


int add_cache_entry(cache_t **cache, char *data, char *URL) {
	//MUST CHECK AVAILABLE CACHE SPACE!
	cache_t *head = *cache;
	//char * temp;
	if(head->length== 0){ //head
		head->content = data;
		head->URL = URL;
		head->next = NULL;
		head->length = (int) (sizeof(data));
		//printf("Saved: \nURL:%s\nPacket:%s\n", head->URL, head->content);
	} else { // append last
		cache_t *iterater = *cache;
		while (iterater->next != NULL) {
			iterater = iterater->next;
		}
		cache_t *newObj = (cache_t *)malloc(sizeof(cache_t));
		newObj->content = data;
		newObj->URL = URL;
		newObj->next = NULL;
		newObj->length = 0;
		iterater->next = newObj;
		//printf("Saved: \nURL:%s\nPacket:%s\n", newObj->URL, newObj->content);
	}
	
	return 0;
}

int enforce_LRU_middle(cache_t **head, char *URL) {
	cache_t *cache = *head;
	cache_t *iterator = cache;
	cache_t *prev = iterator;
	cache_t *last = cache;
	while (last->next != NULL) {
		last = last->next;
	}
	while (iterator != NULL) {
		if (strcmp(iterator->URL, URL) == 0) { //FOUND IT
			if (iterator != last && cache -> next != NULL) {
				//not last and of a cache of size > 1
				prev->next = iterator->next;
				last->next = iterator;
				iterator->next = NULL;
				return 0;
			}
		}
		prev = iterator;
		iterator = iterator->next;
	}
	return 1;
}

cache_t *search_cache(cache_t **cache, char *URL) {
	cache_t *iterator = *cache;
	while (iterator != NULL) {
		if (strcmp(iterator->URL, URL) == 0) {
			return iterator;
		}
		iterator = iterator->next;
	}
	return NULL;
}


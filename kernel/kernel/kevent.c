#include <kernel/kevent.h>
#include <mm/kmalloc.h>
#include <kernel/proc.h>
#include <kstring.h>

static kevent_t* _event_queue_tail = NULL;
static kevent_t* _event_queue_head = NULL;

void kevent_init(void) {
	_event_queue_head = NULL;
	_event_queue_tail = NULL;
}

static kevent_t* new_event(void) {
	kevent_t* event = (kevent_t*)kmalloc(sizeof(kevent_t));
	if(event == NULL) {
		return NULL;
	}
	memset(event, 0, sizeof(kevent_t));
	
	if(_event_queue_tail == NULL) {
		_event_queue_tail = _event_queue_head = event;
	}
	else {
		_event_queue_tail->next = event;
		event->prev = _event_queue_tail;
		_event_queue_tail = event;
	}
	return event;
}

static inline kevent_t* get_event(void) {
	return _event_queue_head;
}

static void remove_event(kevent_t* event) {
	if(event->next != NULL)
		event->next->prev = event->prev;
	if(event->prev != NULL)
		event->prev->next = event->next;

	if(event == _event_queue_head) 
		_event_queue_head = event->next;
	if(event == _event_queue_tail) 
		_event_queue_tail = event->prev;

	kfree(event->data.data);
	kfree(event);
}

int32_t kevent_push(int32_t type, rawdata_t* data) {
	kevent_t* event = new_event();
	if(event == NULL) {
		return -1;
	}

	event->type = type;
	if(data != NULL) {
		event->data.size = data->size;
		event->data.data = kmalloc(data->size);
		if(event->data.data == NULL) {
			kfree(event);
			return -1;
		}
		memcpy(event->data.data, data->data, data->size);
	}
	proc_wakeup((uint32_t)kevent_pop);
	return 0;
}

int32_t kevent_pop(int32_t *type, rawdata_t* data) {
	int32_t res = -1;
	kevent_t* event;
	event = get_event();

	if(event != NULL) {
		*type = event->type;
		data->size = event->data.size;
		data->data = proc_malloc(event->data.size);
		if(data->data != NULL) 
			memcpy(data->data, event->data.data, event->data.size);
		remove_event(event);
		res = 0;
	}
	return res;
}


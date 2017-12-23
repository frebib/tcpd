#include <string.h>
#include <malloc.h>

#include <netstack/frame.h>

struct frame *frame_init(struct intf *intf, void *buffer, size_t buf_size) {
    struct frame *frame = calloc(1, sizeof(struct frame));
    frame->intf = intf;
    if (buffer != NULL)
        frame_init_buf(frame, buffer, buf_size);

    return frame;
}

void frame_init_buf(struct frame* frame, void *buffer, size_t buf_size) {
    frame->buffer = buffer;
    frame->buf_size = buf_size;
    frame->head = frame->buffer;
    frame->tail = frame->buffer + buf_size;
    frame->data = frame->tail;
}

void frame_free(struct frame *frame) {
    if (frame == NULL)
        return;
    if (frame->parent)
        frame->parent->child = NULL;
    // Iterate through children only, we want to keep the parents
    struct frame *child = NULL;
    do {
        child = frame->child;
        free(frame);
        frame = child;
    } while (child != NULL);
}

void frame_parent_free(struct frame *frame) {
    if (frame == NULL)
        return;
    if (frame->child)
        frame->child->parent = NULL;
    // Iterate through children only, we want to keep the parents
    struct frame *parent = NULL;
    do {
        parent = frame->parent;
        free(frame);
        frame = parent;
    } while (parent != NULL);
}

struct frame *frame_child_copy(struct frame *parent) {
    if (parent == NULL) {
        return NULL;
    }

    struct frame *child = malloc(sizeof(struct frame));
    memcpy(child, parent, sizeof(struct frame));

    parent->child = child;
    child->parent = parent;
    child->head = parent->data;
    child->data = child->head;

    return child;
}

struct frame *frame_parent_copy(struct frame *child) {
    if (child == NULL) {
        return NULL;
    }

    struct frame *parent = malloc(sizeof(struct frame));
    memcpy(parent, child, sizeof(struct frame));

    child->parent = parent;
    parent->child = child;
    parent->data = child->head;
    parent->head = child->head;

    return parent;
}

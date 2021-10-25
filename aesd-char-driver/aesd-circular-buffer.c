/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

#define CB_DEBUG 1  //Remove comment on this line to enable debug

#undef PDE1BUG             /* undef it, just in case */
#ifdef CB_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDE1BUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDE1BUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDE1BUG(fmt, args...) /* not debugging: nothing */
#endif

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    
    /* Check for NULL pointers */
    if(buffer == NULL) 
       return NULL;
    
    /* Variable to track position of buffer entry */
    size_t char_count = (buffer->entry[buffer->out_offs]).size;
    int buffer_count  = buffer->out_offs;
    size_t last_pos   = 0;

    PDE1BUG("charoff from cb = %ld, retptr = %p", char_offset, (void *) &buffer->entry[buffer_count]);    
    while(char_offset > (char_count - 1))
    { 
        last_pos     = char_count; 
        buffer_count = (buffer_count + 1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);
        PDE1BUG("1");
        /* Searched all entries and wrapped around, the char_offset exceeds the last byte */
        if(buffer_count == buffer->out_offs)
            return NULL;
        PDE1BUG("2");
        char_count  += (buffer->entry[buffer_count]).size;
    }

    if(char_offset <= (char_count - 1))
    {
        PDE1BUG("3");
        *entry_offset_byte_rtn = char_offset - last_pos;
        return &buffer->entry[buffer_count];
    }
    PDE1BUG("4");
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */

    /* Check for NULL pointers or zero size of entry */
    if(buffer == NULL || add_entry->buffptr == NULL || add_entry->size == 0) 
       return; 

    /* Insert aesd_buffer_entry at in_offs, overwriting data in any case */
    buffer->entry[buffer->in_offs]  = *add_entry;
   
    /* Advance in_offs and out_offs */
    buffer->in_offs = (buffer->in_offs + 1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);
        
    /* If buffer is full, discard the oldest entry by incrementing out_offs */
    if(buffer->full)
         buffer->out_offs = (buffer->out_offs + 1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);

    /* check if in_offs == out_offs -> buffer is full */
    if(buffer->in_offs == buffer->out_offs)
    {
        buffer->full     = 1;
    }   

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
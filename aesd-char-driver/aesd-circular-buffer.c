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
#include <linux/slab.h> //for kfree
#else
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for free
#endif

#include "aesd-circular-buffer.h"

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
    
    /* Variable to track position of buffer entry */
    size_t char_count = (buffer->entry[buffer->out_offs]).size;
    int buffer_count  = buffer->out_offs;
    size_t last_pos   = 0;

    /* Check for NULL pointers */
    if(buffer == NULL) 
       return NULL;
    
    while(char_offset > (char_count - 1))
    { 
        last_pos     = char_count; 
        buffer_count = (buffer_count + 1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED);
        /* Searched all entries and wrapped around, the char_offset exceeds the last byte */
        if(buffer_count == buffer->out_offs)
            return NULL;
        char_count  += (buffer->entry[buffer_count]).size;
    }

    if(char_offset <= (char_count - 1))
    {
        *entry_offset_byte_rtn = char_offset - last_pos;
        return &buffer->entry[buffer_count];
    }
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
* @return NULL or, if an existing entry at the out_offs was replaced,
* the value of buffptr for the entry which was replaced (for use dynamic memory allocation/free)
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */

    const char* freethisptr = NULL;
    /* Check for NULL pointers or zero size of entry */
    if(buffer == NULL || add_entry->buffptr == NULL || add_entry->size == 0) 
       return NULL; 

    if(buffer->full)
        freethisptr = buffer->entry[buffer->in_offs].buffptr;

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

    return freethisptr;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

void aesd_circular_buffer_clean(struct aesd_circular_buffer *buffer)
{
    uint8_t index;
    struct aesd_buffer_entry *entry;

    AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index)
    {
        if(entry->buffptr != NULL)
        {
#ifdef __KERNEL__
            kfree(entry->buffptr);
#else   
             free((char *)entry->buffptr);
#endif 
        }
    }
}

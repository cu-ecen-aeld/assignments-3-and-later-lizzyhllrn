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
#else
#include <string.h>
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

   size_t current_total_bytes = 0;
   size_t current_location= buffer->out_offs; //start at output
   size_t counter=0;

   while (counter <AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
   {
    if ((current_total_bytes + buffer->entry[current_location].size) > char_offset){//the entry is here
        *entry_offset_byte_rtn = char_offset - current_total_bytes;
        //returns entry in buffer where offset occurs
        return &buffer->entry[current_location];

    }
    current_total_bytes =current_total_bytes + buffer->entry[current_location].size;
    current_location = (current_location+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    counter = counter +1;
   }


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
    if(!add_entry || !buffer){
        return;
    }

   buffer->entry[buffer->in_offs] = *add_entry;
   buffer->in_offs=(buffer->in_offs+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

   if (buffer->full) {
    buffer->out_offs=buffer->in_offs;
   }

   buffer->full = (buffer->out_offs==buffer->in_offs);
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

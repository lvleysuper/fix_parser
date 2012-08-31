/* @file   fix_msg.c
   @author Dmitry S. Melnikov, dmitryme@gmail.com
   @date   Created on: 07/30/2012 06:35:11 PM
*/

#include "fix_msg.h"
#include "fix_msg_priv.h"
#include "fix_parser.h"
#include "fix_parser_priv.h"
#include "fix_protocol_descr.h"
#include "fix_tag.h"
#include "fix_types.h"
#include "fix_page.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*-----------------------------------------------------------------------------------------------------------------------*/
/* PUBLICS                                                                                                               */
/*-----------------------------------------------------------------------------------------------------------------------*/
FIXMsg* fix_msg_create(FIXParser* parser, FIXProtocolVerEnum ver, char const* msgType)
{
   if (!parser)
   {
      return NULL;
   }
   fix_parser_reset_error(parser);
   if (!msgType)
   {
      fix_parser_set_error(parser, FIX_ERROR_INVALID_ARGUMENT, "MsgType parameter is NULL");
      return NULL;
   }
   FIXProtocolDescr* prot = fix_parser_get_pdescr(parser, ver);
   if (!prot)
   {
      return NULL;
   }
   FIXMsgDescr* msg_descr = fix_protocol_get_msg_descr(parser, prot, msgType);
   if (!msg_descr)
   {
      return NULL;
   }
   FIXMsg* msg = malloc(sizeof(FIXMsg));
   msg->tags = msg->used_groups = fix_parser_alloc_group(parser);
   if (!msg->tags)
   {
      fix_msg_free(msg);
      return NULL;
   }
   msg->descr = msg_descr;
   msg->parser = parser;
   msg->pages = msg->curr_page = fix_parser_alloc_page(parser, 0);
   if (!msg->pages)
   {
      fix_msg_free(msg);
      return NULL;
   }
   fix_msg_set_string(msg, msg->tags, 8, FIXProtocolVerEnum2BeginString(ver));
   fix_msg_set_string(msg, msg->tags, 35, msgType);
   return msg;
}

/*------------------------------------------------------------------------------------------------------------------------*/
void fix_msg_free(FIXMsg* msg)
{
   FIXPage* page = msg->pages;
   while(page)
   {
      page = fix_parser_free_page(msg->parser, page);
   }
   FIXGroup* grp = msg->used_groups;
   while(grp)
   {
      grp = fix_parser_free_group(msg->parser, grp);

   }
   free(msg);
}

/*------------------------------------------------------------------------------------------------------------------------*/
FIXGroup* fix_msg_alloc_group(FIXMsg* msg)
{
   FIXGroup* grp = fix_parser_alloc_group(msg->parser);
   if (grp)
   {
      grp->next = msg->used_groups;
      msg->used_groups = grp;
   }
   return grp;
}

/*------------------------------------------------------------------------------------------------------------------------*/
void fix_msg_free_group(FIXMsg* msg, FIXGroup* grp)
{
   FIXGroup* curr_grp = msg->used_groups;
   FIXGroup* prev_grp = msg->used_groups;
   while(curr_grp)
   {
      if (curr_grp == grp)
      {
         if (curr_grp == prev_grp)
         {
            msg->used_groups = grp->next;
         }
         else
         {
            prev_grp->next = curr_grp->next;
         }
         fix_parser_free_group(msg->parser, grp);
         break;
      }
      prev_grp = curr_grp;
      curr_grp = curr_grp->next;
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
FIXTag* fix_msg_get_tag(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum)
{
   if (!msg)
   {
      return NULL;
   }
   fix_parser_reset_error(msg->parser);
   if (grp)
   {
      return fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      return fix_tag_get(msg, msg->tags, tagNum);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_del_tag(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   if (grp)
   {
      return fix_tag_del(msg, grp, tagNum);
   }
   else
   {
      return fix_tag_del(msg, msg->tags, tagNum);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
FIXGroup* fix_msg_add_group(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum)
{
   if (!msg)
   {
      return NULL;
   }
   fix_parser_reset_error(msg->parser);
   if (msg->parser->flags & FIXParserFlags_Validate)
   {
      FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
      if (!fdescr)
      {
         return NULL;
      }
      if (fdescr->field_type->type != FIXFieldType_NumInGroup)
      {
         fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' is not a group tag", tagNum);
         return NULL;
      }
   }
   if (grp)
   {
      return fix_group_add(msg, grp, tagNum);
   }
   else
   {
      return fix_group_add(msg, msg->tags, tagNum);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
FIXGroup* fix_msg_get_group(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, uint32_t grpIdx)
{
   if (!msg)
   {
      return NULL;
   }
   fix_parser_reset_error(msg->parser);
   if (msg->parser->flags & FIXParserFlags_Validate)
   {
      FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
      if (!fdescr)
      {
         return NULL;
      }
      if (fdescr->field_type->type != FIXFieldType_NumInGroup)
      {
         fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' is not a group tag", tagNum);
         return NULL;
      }
   }
   if (grp)
   {
      return fix_group_get(msg, grp, tagNum, grpIdx);
   }
   else
   {
      return fix_group_get(msg, msg->tags, tagNum, grpIdx);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_del_group(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, uint32_t grpIdx)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   if (msg->parser->flags & FIXParserFlags_Validate)
   {
      FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
      if (!fdescr)
      {
         return FIX_FAILED;
      }
      if (fdescr->field_type->type != FIXFieldType_NumInGroup)
      {
         fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' is not a group tag", tagNum);
         return FIX_FAILED;
      }
   }
   if (grp)
   {
      return fix_group_del(msg, grp, tagNum, grpIdx);
   }
   else
   {
      return fix_group_del(msg, msg->tags, tagNum, grpIdx);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_set_string(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, char const* val)
{
  if (!msg)
  {
     return FIX_FAILED;
  }
  fix_parser_reset_error(msg->parser);
  if (msg->parser->flags * FIXParserFlags_Validate)
  {
     FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
     if (!fdescr)
     {
        return FIX_FAILED;
     }
     if (!IS_STRING_TYPE(fdescr->field_type->type))
     {
        fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' type is not compatible with value '%s'", tagNum, val);
        return FIX_FAILED;
     }
  }
  FIXTag* tag = fix_msg_set_tag(msg, grp, tagNum, (unsigned char*)val, strlen(val) + 1); // include terminal zero
  return tag != NULL ? FIX_SUCCESS : FIX_FAILED;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_set_int(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, int32_t val)
{
  if (!msg)
  {
     return FIX_FAILED;
  }
  fix_parser_reset_error(msg->parser);
  if (msg->parser->flags & FIXParserFlags_Validate)
  {
     FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
     if (!fdescr)
     {
        return FIX_FAILED;
     }
     if (!IS_INT_TYPE(fdescr->field_type->type))
     {
        fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' type is not compatrible with value '%d'", tagNum, val);
        return FIX_FAILED;
     }
  }
  FIXTag* tag = fix_msg_set_tag(msg, grp, tagNum, (unsigned char*)&val, sizeof(val));
  return tag != NULL ? FIX_SUCCESS : FIX_FAILED;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_set_uint(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, uint32_t val)
{
  if (!msg)
  {
     return FIX_FAILED;
  }
  fix_parser_reset_error(msg->parser);
  if (msg->parser->flags & FIXParserFlags_Validate)
  {
     FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
     if (!fdescr)
     {
        return FIX_FAILED;
     }
     if (!IS_INT_TYPE(fdescr->field_type->type))
     {
        fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' type is not compatrible with value '%u'", tagNum, val);
        return FIX_FAILED;
     }
  }
  FIXTag* tag = fix_msg_set_tag(msg, grp, tagNum, (unsigned char*)&val, sizeof(val));
  return tag != NULL ? FIX_SUCCESS : FIX_FAILED;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_set_int64(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, int64_t val)
{
  if (!msg)
  {
     return FIX_FAILED;
  }
  fix_parser_reset_error(msg->parser);
  if (msg->parser->flags & FIXParserFlags_Validate)
  {
     FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
     if (!fdescr)
     {
        return FIX_FAILED;
     }
     if (!IS_INT_TYPE(fdescr->field_type->type))
     {
        fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' type is not compatrible with value '%ld'", tagNum, val);
        return FIX_FAILED;
     }
  }
  FIXTag* tag = fix_msg_set_tag(msg, grp, tagNum, (unsigned char*)&val, sizeof(val));
  return tag != NULL ? FIX_SUCCESS : FIX_FAILED;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_set_uint64(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, uint64_t val)
{
  if (!msg)
  {
     return FIX_FAILED;
  }
  fix_parser_reset_error(msg->parser);
  if (msg->parser->flags & FIXParserFlags_Validate)
  {
     FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
     if (!fdescr)
     {
        return FIX_FAILED;
     }
     if (!IS_INT_TYPE(fdescr->field_type->type))
     {
        fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' type is not compatrible with value '%lu'", tagNum, val);
        return FIX_FAILED;
     }
  }
  FIXTag* tag = fix_msg_set_tag(msg, grp, tagNum, (unsigned char*)&val, sizeof(val));
  return tag != NULL ? FIX_SUCCESS : FIX_FAILED;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_set_char(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, char val)
{
  if (!msg)
  {
     return FIX_FAILED;
  }
  fix_parser_reset_error(msg->parser);
  if (msg->parser->flags & FIXParserFlags_Validate)
  {
     FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
     if (!fdescr)
     {
        return FIX_FAILED;
     }
     if (!IS_CHAR_TYPE(fdescr->field_type->type))
     {
        fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' type is not compatrible with value '%c'", tagNum, val);
        return FIX_FAILED;
     }
  }
  FIXTag* tag = fix_msg_set_tag(msg, grp, tagNum, (unsigned char*)&val, 1);
  return tag != NULL ? FIX_SUCCESS : FIX_FAILED;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_set_float(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, float val)
{
  if (!msg)
  {
     return FIX_FAILED;
  }
  fix_parser_reset_error(msg->parser);
  if (msg->parser->flags & FIXParserFlags_Validate)
  {
     FIXFieldDescr* fdescr = fix_protocol_get_field_descr(msg->parser, msg->descr, tagNum);
     if (!fdescr)
     {
        return FIX_FAILED;
     }
     if (!IS_FLOAT_TYPE(fdescr->field_type->type))
     {
        fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag '%d' type is not compatrible with value '%f'", tagNum, val);
        return FIX_FAILED;
     }
  }
  FIXTag* tag = fix_msg_set_tag(msg, grp, tagNum, (unsigned char*)&val, sizeof(val));
  return tag != NULL ? FIX_SUCCESS : FIX_FAILED;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_get_int(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, int32_t* val)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXTag* tag = NULL;
   if (grp)
   {
      tag = fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      tag = fix_tag_get(msg, msg->tags, tagNum);
   }
   if (!tag)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' not found", tagNum);
      return FIX_FAILED;
   }
   if (tag->type != FIXTagType_Value)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag %d is not a value", tagNum);
      return FIX_FAILED;
   }
   *val = *(int32_t*)tag->data;
   return FIX_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_get_uint(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, uint32_t* val)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXTag* tag = NULL;
   if (grp)
   {
      tag = fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      tag = fix_tag_get(msg, msg->tags, tagNum);
   }
   if (!tag)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' not found", tagNum);
      return FIX_FAILED;
   }
   if (tag->type != FIXTagType_Value)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag %d is not a value", tagNum);
      return FIX_FAILED;
   }
   *val = *(uint32_t*)tag->data;
   return FIX_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_get_int64(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, int64_t* val)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXTag* tag = NULL;
   if (grp)
   {
      tag = fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      tag = fix_tag_get(msg, msg->tags, tagNum);
   }
   if (!tag)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' not found", tagNum);
      return FIX_FAILED;
   }
   if (tag->type != FIXTagType_Value)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag %d is not a value", tagNum);
      return FIX_FAILED;
   }
   *val = *(int64_t*)tag->data;
   return FIX_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_get_uint64(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, uint64_t* val)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXTag* tag = NULL;
   if (grp)
   {
      tag = fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      tag = fix_tag_get(msg, msg->tags, tagNum);
   }
   if (!tag)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' not found", tagNum);
      return FIX_FAILED;
   }
   if (tag->type != FIXTagType_Value)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag %d is not a value", tagNum);
      return FIX_FAILED;
   }
   *val = *(uint64_t*)tag->data;
   return FIX_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_get_float(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, float* val)
{
   if(!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXTag* tag = NULL;
   if (grp)
   {
      tag = fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      tag = fix_tag_get(msg, msg->tags, tagNum);
   }
   if (!tag)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' not found", tagNum);
      return FIX_FAILED;
   }
   if (tag->type != FIXTagType_Value)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag %d is not a value", tagNum);
      return FIX_FAILED;
   }
   *val = *(float*)tag->data;
   return FIX_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_get_char(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, char* val)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXTag* tag = NULL;
   if (grp)
   {
      tag = fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      tag = fix_tag_get(msg, msg->tags, tagNum);
   }
   if (!tag)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' not found", tagNum);
      return FIX_FAILED;
   }
   if (tag->type != FIXTagType_Value)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag %d is not a value", tagNum);
      return FIX_FAILED;
   }
   *val = *(char*)tag->data;
   return FIX_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_get_string(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, char* val, uint32_t len)
{
   if (!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXTag* tag = NULL;
   if (grp)
   {
      tag = fix_tag_get(msg, grp, tagNum);
   }
   else
   {
      tag = fix_tag_get(msg, msg->tags, tagNum);
   }
   if (!tag)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' not found", tagNum);
      return FIX_FAILED;
   }
   if (tag->type != FIXTagType_Value)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_TAG_HAS_WRONG_TYPE, "Tag %d is not a value", tagNum);
      return FIX_FAILED;
   }
   strncpy(val, (char const*)tag->data, len);
   uint32_t const srclen = strlen((char const*)tag->data);
   return srclen > len ? len : srclen;
}

/*------------------------------------------------------------------------------------------------------------------------*/
int fix_msg_to_string(FIXMsg* msg, char delimiter, char* buff, uint32_t buffLen)
{
   if(!msg)
   {
      return FIX_FAILED;
   }
   fix_parser_reset_error(msg->parser);
   FIXMsgDescr* descr = msg->descr;
   for(uint32_t i = 0; i < descr->field_count; ++i)
   {
      FIXFieldDescr* field = &descr->fields[i];
      FIXTag* tag = fix_tag_get(msg, NULL, field->field_type->num);
      if (!tag && field->flags & FIELD_FLAG_REQUIRED)
      {
         fix_parser_set_error(msg->parser, FIX_ERROR_TAG_NOT_FOUND, "Tag '%d' is required", field->field_type->num);
      }
      int res = 0;
      if (IS_STRING_TYPE(field->field_type->type))
      {
         res = snprintf(buff, buffLen, "%d=%s", field->field_type->num, (char const*)tag->data);
      }
      else if (IS_INT_TYPE(field->field_type->type))
      {
         res = snprintf(buff, buffLen, "%d=%ld\001", field->field_type->num, *(long*)tag->data);
      }
      else if (IS_CHAR_TYPE(field->field_type->type))
      {
         res = snprintf(buff, buffLen, "%d=%c\001", field->field_type->num, *(char*)tag->data);
      }
      else if (IS_FLOAT_TYPE(field->field_type->type))
      {
         res = snprintf(buff, buffLen, "%d=%f\001", field->field_type->num, *(float*)tag->data);
      }
      // TODO: check res for errors
      buff += res;
      buffLen -= res;
   }
   return FIX_SUCCESS;
}

/*------------------------------------------------------------------------------------------------------------------------*/
/* PRIVATES                                                                                                               */
/*------------------------------------------------------------------------------------------------------------------------*/

void* fix_msg_alloc(FIXMsg* msg, uint32_t size)
{
   FIXPage* curr_page = msg->curr_page;
   if (sizeof(uint32_t) + curr_page->size - curr_page->offset >= size)
   {
      uint32_t old_offset = curr_page->offset;
      *(uint32_t*)(&curr_page->data + curr_page->offset) = size;
      curr_page->offset += (size + sizeof(uint32_t));
      return &curr_page->data + old_offset + sizeof(uint32_t);
   }
   else
   {
      FIXPage* new_page = fix_parser_alloc_page(msg->parser, size);
      if (!new_page)
      {
         return NULL;
      }
      curr_page->next = new_page;
      msg->curr_page = new_page;
      return fix_msg_alloc(msg, size);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
void* fix_msg_realloc(FIXMsg* msg, void* ptr, uint32_t size)
{
   if (*(uint32_t*)(ptr - sizeof(uint32_t)) >= size)
   {
      return ptr;
   }
   else
   {
      return fix_msg_alloc(msg, size);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
FIXTag* fix_msg_set_tag(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, unsigned char const* data, uint32_t len)
{
   if (grp)
   {
      return fix_tag_set(msg, grp, tagNum, data, len);
   }
   else
   {
      return fix_tag_set(msg, msg->tags, tagNum, data, len);
   }
}

/*------------------------------------------------------------------------------------------------------------------------*/
FIXTag* set_tag_fmt(FIXMsg* msg, FIXGroup* grp, uint32_t tagNum, char const* fmt, ...)
{
   int n, size = 64;
   char *p, *np;
   va_list ap;

   if ((p = malloc(size)) == NULL)
   {
      fix_parser_set_error(msg->parser, FIX_ERROR_MALLOC, "Unable to allocate %d bytes", size);
      return NULL;
   }

   while (1)
   {
      va_start(ap, fmt);
      n = vsnprintf(p, size, fmt, ap);
      va_end(ap);

      if (n < 0)
      {
         fix_parser_set_error(msg->parser, FIX_ERROR_INVALID_ARGUMENT, "Unable to set tag. Wrong format '%s'", fmt);
         return NULL;
      }

      if (n < size)
      {
         FIXTag* tag = NULL;
         if (grp)
         {
            tag = fix_tag_set(msg, grp, tagNum, (unsigned char*)p, n);
         }
        else
         {
            tag = fix_tag_set(msg, msg->tags, tagNum, (unsigned char*)p, n);
         }
         free(p);
         return tag;
      }

      if (size == n)
      {
         size *= 2;
      }

      if ((np = realloc (p, size)) == NULL)
      {
         free(p);
         fix_parser_set_error(msg->parser, FIX_ERROR_MALLOC, "Unable to allocate %d bytes", size);
         return NULL;
      }
      else
      {
         p = np;
      }
   }
}
#include "block.h"

#include "functions.h"
block* block::operator[](int i)
{
    if(i > childBlockLength || i < 0) return NULL;
    else
    {
        return childBlock[i];
    }
}
bool block::add_childBlock(unsigned int count, int init_length)
{
    if(this->childBlock != NULL) return false;
    else 
    {
        childBlock = new block*[count];
        childBlockLength = count;

        for(int i = 0; i < count; i++) {childBlock[i] = new block(init_length);}
        return true;
    }
}
bool block::add_childBlock(unsigned int count)
{
    if(this->childBlock != NULL) return false;
    else 
    {
        childBlock = new block*[count];
        childBlockLength = count;
        for(int i = 0; i < count; i++) {childBlock[i] = new block();}
        return true;
    }
}
block* block::get_childBlock(int index)
{
    if(childBlock == NULL) return NULL;
    else return childBlock[index];
}
bool block::set_blockValue(int i, int value)
{
    if(this->value == NULL) return false;
    else {this->value[i] = value; return true;}
}

void block::freeblock(block* blToFree)
{
    if(blToFree->childBlock) 
    {
        for(size_t i = 0; i < blToFree->childBlockLength; i++) 
        {
            if(blToFree->childBlock && blToFree->childBlock[i])
            freeblock(blToFree->childBlock[i]);
        }
    }

    Sdelete_l(value);
    Sdelete_l(childBlock);
}
block::block(int a)
{
    value = new int[a];
    childBlock = NULL;
    childBlockLength = 0;
    length = a;
    coded_block_flag = 0;
}
block::block()
{
    value = NULL;
    childBlock = NULL;
    childBlockLength = 0;
    length = 0;
    coded_block_flag = 0;
}
block::~block()
{
    if(this->childBlock) 
    {
        for(size_t i = 0; i < this->childBlockLength; i++) 
        {
            Sdelete_s(this->childBlock[i]);
        }
    }
    Sdelete_l(childBlock);
    Sdelete_l(value);
}
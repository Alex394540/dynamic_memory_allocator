#include <stdbool.h>
#include <string.h>


struct BlockHeader {
    bool is_free;
    bool is_start;
    bool is_end;
    std::size_t length;
};


struct DataBlock {
    BlockHeader block_start;
    void * data;
    BlockHeader block_end;
};


struct DataBlock * first_data_block;


DataBlock * next_block(DataBlock * current)
{
    // next block or null...
    std::size_t cur_length = current->block_start.length;
    if (current->block_end.is_end) return NULL;

    // Get pointer to the next block start ...
    void * start_of_next_datablock = (void *) current + current->block_start.length + 2 * sizeof(struct BlockHeader);
    DataBlock * next_data_block = (DataBlock *) start_of_next_datablock;
    BlockHeader * block_start = (BlockHeader *) start_of_next_datablock;
    
    void * data = (start_of_next_datablock + sizeof(struct BlockHeader));
    BlockHeader * block_end = (BlockHeader *) (data + block_start->length);

    next_data_block->block_start = *block_start;
    next_data_block->data = data;
    next_data_block->block_end = *block_end;

    return next_data_block;
}


DataBlock * prev_block(DataBlock * current)
{
    // prev block or null...
    if (current->block_start.is_start) return NULL;
    BlockHeader * prev_end = (BlockHeader *) ((void*)(&current->block_start) - sizeof(struct BlockHeader));
    std::size_t prev_length = prev_end->length;

    void * data = (void *)prev_end - prev_length;
    BlockHeader * prev_start = (BlockHeader *) (data - sizeof(struct BlockHeader));

    DataBlock * prev_data_block = (DataBlock *) ((void *) prev_start);
    prev_data_block->block_start = *prev_start;
    prev_data_block->data = data;
    prev_data_block->block_end = *prev_end;

    return prev_data_block;
}


void mysetup(void *buf, std::size_t size)
{
    std::size_t useful_size = size - 2 * sizeof(struct BlockHeader);
    BlockHeader start_header = {true, true, false, useful_size};
    memcpy(buf, &start_header, sizeof(struct BlockHeader));

    void * data = buf + sizeof(struct BlockHeader);

    BlockHeader end_header = {true, false, true, useful_size};
    memcpy(buf + size - sizeof(struct BlockHeader), &end_header, sizeof(struct BlockHeader));

    first_data_block = (DataBlock *) buf;
    first_data_block -> block_start = start_header;
    first_data_block -> data = data;
    first_data_block -> block_end = end_header;
}


// Функция аллокации
void *myalloc(std::size_t size)
{
    // Ищем первый подходящий по размеру участок памяти
    DataBlock * current = first_data_block;
    while (current != NULL)
    {
        if (current->block_start.is_free && current->block_start.length >= size)
        {
            // Запомним его полный размер
            std::size_t old_size = current->block_start.length;

            current->block_start.is_free = false;

            // Проверим, что оставшееся расстояние больше, чем 2 блока
            if (old_size - size >= 2 * sizeof(struct BlockHeader))
            {
                BlockHeader big_segment_end_header = current->block_end;

                // Отрезаем от него нужную часть
                current->block_start.length = size;
                BlockHeader finish = {false, false, false, size};
                void * end_block_addr = memcpy((void*)current + size + sizeof(struct BlockHeader), &finish, sizeof(struct BlockHeader));
                current->block_end = finish;
                // Добавляем метаинформацию для оставшегося блока
                BlockHeader new_block_start = {true, false, false, old_size - size - 2 * sizeof(struct BlockHeader)};
                void * new_start_block_addr = memcpy(end_block_addr + sizeof(struct BlockHeader), &new_block_start, sizeof(struct BlockHeader));

                // Добавляем концевик
                BlockHeader new_block_end = {true, false, big_segment_end_header.is_end, old_size - size - 2 * sizeof(struct BlockHeader)};
                memcpy(new_start_block_addr + old_size - size - sizeof(struct BlockHeader), &new_block_end, sizeof(struct BlockHeader));

            }
            // Иначе нет смысла оставлять этот кусок свободным
            else
            {
                current->block_end.is_free = false;
            }
            return current->data;
        }
        else
        {
            current = next_block(current);
        }
    }

    return NULL;
}


// Функция освобождения
void myfree(void *p)
{
    BlockHeader * block_start = (BlockHeader *) (p - sizeof(struct BlockHeader));
    std::size_t region_length = block_start->length;
    BlockHeader * block_end = (BlockHeader *) (p + region_length);

    block_start->is_free = true;
    block_end->is_free = true;

    DataBlock * current = (DataBlock *) block_start;
    current->block_start = *block_start;
    current->data = p;
    current->block_end = *block_end;

    DataBlock * prev = prev_block(current);
    DataBlock * next = next_block(current);

    // If prev block is free
    if (prev != NULL && prev->block_start.is_free) {
        
        // Change length - prev_start, cur_end
        std::size_t new_length = prev->block_start.length + current->block_start.length + 2 * sizeof(struct BlockHeader);
        prev->block_start.length = new_length;

        BlockHeader prev_block_end = {true, false, current->block_end.is_end, new_length};
        memcpy((void *)prev + new_length + sizeof(struct BlockHeader), &prev_block_end, sizeof(struct BlockHeader));
        prev->block_end = prev_block_end;
        // Shift current block pointer
        current = prev;
    }

    // Now check the next one
    if (next != NULL && next->block_start.is_free) {
        // Change prev_start length and next_end length
        std::size_t new_length = next->block_start.length + current->block_start.length + 2 * sizeof(struct BlockHeader);
        current->block_start.length = new_length;

        BlockHeader cur_block_end = {true, false, next->block_end.is_end, new_length};
        memcpy((void*) current + new_length + sizeof(struct BlockHeader), &cur_block_end, sizeof(struct BlockHeader));
        current->block_end = cur_block_end;
    }
}

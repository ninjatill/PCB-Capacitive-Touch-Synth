#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/sync.h"

#include "audio_commands.h"

static constexpr uint8_t AUDIO_COMMAND_QUEUE_SIZE = 32;

static AudioCommand queue[AUDIO_COMMAND_QUEUE_SIZE];

static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;

static uint8_t next_index(uint8_t index)
{
    return (uint8_t)((index + 1) % AUDIO_COMMAND_QUEUE_SIZE);
}

bool audio_commands_push(const AudioCommand& command)
{
    uint32_t irq_state = save_and_disable_interrupts();

    uint8_t next_head = next_index(head);

    if (next_head == tail) {
        restore_interrupts(irq_state);
        printf("AUDIO CMD: queue full, command dropped type=%d\n", command.type);
        return false;
    }

    queue[head] = command;
    head = next_head;

    restore_interrupts(irq_state);

    return true;
}

bool audio_commands_pop(AudioCommand* command)
{
    if (command == nullptr) {
        return false;
    }

    uint32_t irq_state = save_and_disable_interrupts();

    if (head == tail) {
        restore_interrupts(irq_state);
        return false;
    }

    *command = queue[tail];
    tail = next_index(tail);

    restore_interrupts(irq_state);

    return true;
}
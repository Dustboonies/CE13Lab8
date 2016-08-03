/**
 * This library implements functions for decoding Morse code. It builds upon the Tree library to use
 * a binary tree data structure for storing the codepoints for all characters. Decoding is then done
 * through simple tree traversal. Additional functionality relies on a 100Hz clock to check the
 * button states for decoding the input of Morse characters through a physical button (BTN4).
 */

#include <stdint.h>
#include "Morse.h"
#include "Tree.h"
#include "Buttons.h"
#include "BOARD.h"


Node *chartree;
Node *temp;

enum {
    WAITING,
    DOT,
    DASH,
    INTER_LETTER
} Mstate = WAITING;

/**
 * This function initializes the Morse code decoder. This is primarily the generation of the
 * Morse tree: a binary tree consisting of all of the ASCII alphanumeric characters arranged
 * according to the DOTs and DASHes that represent each character. Traversal of the tree is done by
 * taking the left-child if it is a dot and the right-child if it is a dash. If the morse tree is
 * successfully generated, SUCCESS is returned, otherwise STANDARD_ERROR is returned. This function
 * also initializes the Buttons library so that MorseCheckEvents() can work properly.
 * @return Either SUCCESS if the decoding tree was successfully created or STANDARD_ERROR if not.
 */
int MorseInit(void)
{
    ButtonsInit();
    char tree[63] = {'\0', 'E', 'I', 'S', 'H', '5', '4', 'V', '\0', '3', 'U', 'F', '\0', '\0', '\0', '\0', '2',
        'A', 'R', 'L', '\0', '\0', '\0', '\0', '\0', 'W', 'P', '\0', '\0', 'J', '\0', '1',
        'T', 'N', 'D', 'B', '6', '\0', 'X', '\0', '\0', 'K', 'C', '\0', '\0', 'Y', '\0', '\0',
        'M', 'G', 'Z', '7', '\0', 'Q', '\0', '\0', 'O', '\0', '8', '\0', '\0', '9', '0'};
    chartree = TreeCreate(6, tree);
    if (chartree == NULL) {
        return STANDARD_ERROR;
    } else {
        temp = chartree;
        return SUCCESS;
    }
}

/**
 * MorseDecode decodes a Morse string by iteratively being passed MORSE_CHAR_DOT or MORSE_CHAR_DASH.
 * Each call that passes a DOT or DASH returns a SUCCESS if the string could still compose a
 * Morse-encoded character. Passing in the special MORSE_CHAR_END_OF_CHAR constant will terminate
 * decoding and return the decoded character. During that call to MorseDecode() the return value
 * will be the character that was decoded or STANDARD_ERROR if it couldn't be decoded. Another
 * special value exists, MORSE_CHAR_DECODE_RESET, which will clear the stored state. When a
 * MORSE_CHAR_DECODE_RESET is done, SUCCESS will be returned. If the input is not a valid MorseChar
 * then the internal state should be reset and STANDARD_ERROR should be returned.
 * 
 * @param in A value from the MorseChar enum which specifies how to traverse the Morse tree.
 * 
 * @return Either SUCCESS on DECODE_RESET or when the next traversal location is still a valid
 *         character, the decoded character on END_OF_CHAR, or STANDARD_ERROR if the Morse tree
 *         hasn't been initialized, the next traversal location doesn't exist/represent a character,
 *         or `in` isn't a valid member of the MorseChar enum.
 */
char MorseDecode(MorseChar in)
{
    char tempchar;
    if (in == MORSE_CHAR_DOT) {
        if (temp->leftChild == NULL) {
            return STANDARD_ERROR;
        }
        temp = temp->leftChild;
        return SUCCESS;
    } else if (in == MORSE_CHAR_DASH) {
        if (temp->rightChild == NULL) {
            return STANDARD_ERROR;
        }
        temp = temp->rightChild;
        return SUCCESS;
    } else if (in == MORSE_CHAR_END_OF_CHAR) {
        tempchar = temp->data;
        temp = chartree;
        return tempchar;
    } else if (in == MORSE_CHAR_DECODE_RESET) {
        temp = chartree;
        return SUCCESS;
    } else {
        return STANDARD_ERROR;
    }
    return SUCCESS;
}

/**
 * This function calls ButtonsCheckEvents() once per call and returns which, if any,
 * of the Morse code events listed in the enum above have been encountered. It checks for BTN4
 * events in its input and should be called at 100Hz so that the timing works. The
 * length that BTN4 needs to be held down for a dot is >= 0.25s and < 0.50s with a dash being a button
 * down event for >= 0.5s. The button uptime various between dots/dashes (>= .5s), letters
 * (>= 1s), and words (>= 2s) Letters > 1s; words greater than 2s.
 *
 * @note This function assumes that the buttons are all unpressed at startup, so that the first
 *       event it will see is a BUTTON_EVENT_*DOWN.
 *
 * So pressing the button for 0.1s, releasing it for 0.1s, pressing it for 0.3s, and then waiting
 * will decode the string '.-' (A). It will trigger the following order of events:
 * 9 MORSE_EVENT_NONEs, 1 MORSE_EVENT_DOT, 39 MORSE_EVENT_NONEs, a MORSE_EVENT_DASH, 69
 * MORSE_EVENT_NONEs, a MORSE_EVENT_END_CHAR, and then MORSE_EVENT_INTER_WORDs.
 * 
 * @return The MorseEvent that occurred.
 */

uint8_t first = 0;
int increment = 0;

MorseEvent MorseCheckEvents(void)
{
    increment++;
    first = ButtonsCheckEvents();
    switch (Mstate) {
    case WAITING:
        increment = 0;
        if (first == BUTTON_EVENT_4DOWN) {
            printf("Button 4 Pressed");
            Mstate = DOT;
        } else {
            Mstate = WAITING;
        }
        break;
    case DOT:
        if (first == BUTTON_EVENT_4UP) {
            printf("Button 4 Up");
            increment = 0;
            Mstate = INTER_LETTER;
            return MORSE_EVENT_DOT;
        }
        if (increment > MORSE_EVENT_LENGTH_DOWN_DASH) {
            Mstate = DASH;
        }
        break;
    case DASH:
        if (first == BUTTON_EVENT_4UP) {
            printf("Button 4 Up");
            increment = 0;
            Mstate = INTER_LETTER;
            return MORSE_EVENT_DASH;
        }
        break;
    case INTER_LETTER:
        if (increment >= MORSE_EVENT_LENGTH_UP_INTER_LETTER) {
            Mstate = WAITING;
            return MORSE_EVENT_INTER_WORD;
        }
        if (first == BUTTON_EVENT_4DOWN) {
            if (increment >= MORSE_EVENT_LENGTH_UP_INTER_LETTER) {
                increment = 0;
                Mstate = DOT;
                return MORSE_EVENT_INTER_LETTER;
            }
            increment = 0;
            Mstate = DOT;
        }
        break;
    }
    return MORSE_EVENT_NONE;

}
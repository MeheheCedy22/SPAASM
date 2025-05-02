# Pseudocode for main functionality of the program
Note: This is nearly the same as the main code, but with some small changes that I made directly in the code so I don't have to write it here again

data segment:
 <!-- - flag to find out if the word starting with uppercase is found -->
 - counter - how many lines
 - offset to start of the line


pseudo code:
```py
while not end of file:
    load new buffer

    offset = 0 (start of buffer)
    while not end of buffer:

        get char
        
        if (char is uppercase and offset == 0) or (char is uppercase and [char-1] is whitespace):
            counter++

            while not end of line:

                get next char

                if char is newline:
                    add '$' to the end of line
                    print(offset, current)
                    offset = current + 1
                    break
        else if char is newline:
           offset = current + 1


        before loading new buffer, find last newline in loaded buffer and offset the cursor by the delta 
```
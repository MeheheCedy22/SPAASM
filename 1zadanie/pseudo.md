# Pseudokod na jadro programu

data segment:
 <!-- - flag na zistenie ci je found slovo ktore zacina na uppercase -->
 - counter vyskytov riadkov
 - offset na zaciatok riadku


pseudo code


```py
while not end of file:

    offset = 0 (start of buffer)
    while not end of buffer:

        get char
        
        # if flag == 0:
            if (char is uppercase and if [char-1] is whitespace) or (char is uppercase and offset == 0):
                # flag = 1
                counter++

                while not end of line:

                    get next char

                    if char is newline:
                        add '$' to the end of line
                        print(offset, current)
                        offset = current + 1
                        break
        else:
            while not end of line:

                if char is newline:
                    # flag = 0
                    offset = current + 1
                else:
                    get next char
```
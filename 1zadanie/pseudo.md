# Pseudokod na jadro programu
Note: je to skoro to iste len s nejakymi malymi zmenami ktore som robil priamo v kode aby som to nemusel pisat este aj sem

data segment:
 <!-- - flag na zistenie ci je found slovo ktore zacina na uppercase -->
 - counter vyskytov riadkov
 - offset na zaciatok riadku


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
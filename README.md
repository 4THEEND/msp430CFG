# MSP430 CFG

This tool is used to disassemble msp430 binaries using recursive traversal technique and generate a cfg out of it.

Once you have used the command export to get the `.dot` file you can then export it to the `.png` format using the following command:

`dot -Tpng ./cfg.dot -o ./cfg.PNG`.

It supports loading multiplie binairies using the `load` command (and `select` to select which binairy you want to work with).

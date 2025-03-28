# SecureWipe
An extremely secure command line data wiping program using standard linux C libraries

# Compiling
```
gcc secureWipe.c -o secureWipe
```
# Commandline Arguments
```
./secureWipe <drivename> <amount of itterations>
```
# Wiping Method
My wiping program
1. Writes random data to the entire drive
2. Writes 10101010 to the entire drive
3. Writes 01010101 to the entire drive
These happen as many times as specified in the command line argument

After specified itterations
1. It writes all 1's to the entire drive
2. It writes all 0's to the entire drive

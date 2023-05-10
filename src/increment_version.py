import os

# Read the current version number from version.txt
with open("src/version.txt", "r") as f:
    version = int(f.read().strip())

# Increment the version number
version += 1

# Write the updated version number back to version.txt
with open("src/version.txt", "w") as f:
    f.write(str(version))

# Write the updated version number to version.h
with open("src/version.h", "w") as f:
    f.write("#define ECHO_VERSION " + str(version))

print(version)
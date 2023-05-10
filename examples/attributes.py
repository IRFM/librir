from librir.tools.FileAttributes import FileAttributes

# Create a FileAttributes object over 'my_file'.
# Create 'my_file' is it doesn't exist, otherwise read existing attributes from 'my_file' or create an attributes trailer if non is found.
f = FileAttributes("my_file")

# define some global attributes
f.attributes["attr1"] = 1
f.attributes["attr2"] = "a value"

# set timestamps BEFORE setting frame attributes
f.timestamps = [0, 1, 2, 3]

# set frame attributes for frame 0
f.set_frame_attributes(0, {"attr1": 1, "attr2": "a value"})

# write attributes
f.close()

#
# read back
#


f = FileAttributes("my_file")

print(f.attributes)
print(f.timestamps)
print(f.frame_attributes(0))

compiler = g++
bin = ATPG
src = $(shell find ./src -name "*.cpp")
obj = $(src:%.c=%.o)
inc = -Iinc -Iglucose -Iglucose/utils -Iglucose/core -Iglucose/simp
glucoseLibDir = -Lglucose/simp
glucoseLib = -l_release
#-Iinc means find the head file at "inc" folder
$(bin): $(obj)
	$(compiler) -o $(bin) $(obj) $(inc)  $(glucoseLibDir) $(glucoseLib)

%.o: %.cpp
	$(compiler) -c $< -o $@

clean:
	rm -rf $(bin)

#to make the glucose Library
# first step:  cd simp
# second step: make libr

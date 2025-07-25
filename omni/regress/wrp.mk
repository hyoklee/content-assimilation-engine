.PHONY: all $(OMNIS)

# Define your SSH hosts here (e.g., user@hostname or user@IP)
OMNIS := tf1 tf2 tf3 tf4

# Define the command to execute on remote hosts
COMMAND ?= "wrt"

# Default target: depends on all host-specific targets
all: $(HOSTS)

# Rule to define how to execute a command on a single host
# The $@ variable represents the current target (which will be a host name)
%:
	@echo "--- Connecting to $@ ---"
	@ssh -T $@ $(COMMAND) put $@.yml
	@echo "--- Command completed on $@ ---"

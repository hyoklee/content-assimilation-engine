Let's create a jarvis package for running an omni script. The package should be located in the repo test/jarvis_wrp_cae. Call the package omni_parse. Look at the omni/README.md to see how to deploy the example script. The jarvis package should take as input a omni yaml file.

The package should execute the omni script locally using a LocalExecInfo. It should use mod_env for the environment because we want to support interception. 

Attached is the context for jarvis: 
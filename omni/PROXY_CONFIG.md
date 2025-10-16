# Proxy Configuration for POCO HTTP Calls

## Overview

The OMNI/wrp tool now supports HTTP/HTTPS proxy configuration for network operations including:
- File downloads via `Download()` function
- Globus transfers via `httpGet()` and `httpPost()` functions
- DataHub API calls

## Configuration

### Setup

1. Create the configuration directory:
   ```bash
   mkdir -p ~/.wrp
   ```

2. Create or edit `~/.wrp/config`:
   ```bash
   vi ~/.wrp/config
   ```

3. Add proxy configuration:
   ```
   ProxyConfig enabled
   ProxyHost proxy.example.com
   ProxyPort 8080
   ```

### Configuration Options

- **ProxyConfig enabled**: Enables proxy support. Without this line, proxy is disabled.
- **ProxyHost <hostname>**: The proxy server hostname or IP address
- **ProxyPort <port>**: The proxy server port number

### Example Configurations

#### Enable Proxy
```
ProxyConfig enabled
ProxyHost webproxy.myorg.com
ProxyPort 8080
```

#### Disable Proxy
Simply comment out or remove the `ProxyConfig enabled` line:
```
# ProxyConfig enabled
ProxyHost webproxy.myorg.com
ProxyPort 8080
```

Or don't create the config file at all.

#### Combined with DataHub
```
ProxyConfig enabled
ProxyHost proxy.example.com
ProxyPort 8080

MetaStore DataHub
```

## Testing

### Running Tests with Proxy

The `poco.sh` script runs tests that require network access. The following tests will use proxy settings if configured:

- **http**: Tests basic HTTP download
- **redi**: Tests HTTP redirect handling
- **rget**: Tests HTTP range requests

To test:
```bash
./poco.sh
```

### Verify Proxy Usage

When proxy is enabled, the tool will print messages like:
```
Proxy configured: proxy.example.com:8080
Using proxy: proxy.example.com:8080
```

## Implementation Details

### Modified Files

- **OMNI.h**: Added `ProxyConfig` struct and `ReadProxyConfig()` method
- **OMNI.cc**:
  - Implemented `ReadProxyConfig()` to parse ~/.wrp/config
  - Added overloaded `Download()` method accepting ProxyConfig
  - Modified HTTP session setup to use proxy when enabled
- **glo.cc**:
  - Added `readProxyConfigForGlobus()` helper function
  - Modified `httpGet()` and `httpPost()` to use proxy settings

### Proxy Support

The proxy configuration uses POCO's built-in proxy support:
- `HTTPClientSession::setProxyHost()`
- `HTTPClientSession::setProxyPort()`

When proxy is disabled or not configured, the tool explicitly disables proxy to avoid system proxy settings that might cause DNS issues:
```cpp
session->setProxyHost("");
session->setProxyPort(0);
```

## Troubleshooting

### Tests Still Failing

If tests still fail after configuring proxy:

1. **Verify config file location**: Ensure `~/.wrp/config` exists and is readable
   ```bash
   ls -la ~/.wrp/config
   cat ~/.wrp/config
   ```

2. **Check proxy settings**: Verify the proxy host and port are correct
   ```bash
   curl -x http://proxy.example.com:8080 https://github.com
   ```

3. **Check network connectivity**: Ensure the proxy is reachable
   ```bash
   nc -zv proxy.example.com 8080
   ```

4. **Run tests with verbose output**:
   ```bash
   cd poco
   ctest --rerun-failed --output-on-failure
   ```

### Authentication

Currently, proxy authentication (username/password) is not supported. If your proxy requires authentication, you may need to:
- Use a proxy that doesn't require authentication
- Configure authentication at the system level
- Or extend the implementation to add authentication support

## Future Enhancements

Potential improvements:
- Proxy authentication support (username/password)
- Different proxy settings for HTTP vs HTTPS
- Proxy bypass list for certain domains
- Environment variable support (HTTP_PROXY, HTTPS_PROXY)

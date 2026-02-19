import { config, getBaseUrl } from './config.js';

interface PluginResponse<T = unknown> {
  success: boolean;
  data?: T;
  error?: {
    code: number;
    message: string;
  };
}

type ConnectionState = 'connected' | 'disconnected' | 'reconnecting';

export class HttpClient {
  private base_url: string;
  private state: ConnectionState = 'disconnected';
  private last_successful_request = 0;
  private consecutive_failures = 0;
  private health_check_interval: ReturnType<typeof setInterval> | null = null;

  constructor() {
    this.base_url = getBaseUrl();
    this.start_health_monitor();
  }

  get connection_state(): ConnectionState {
    return this.state;
  }

  async get<T = unknown>(path: string, params?: Record<string, string>): Promise<T> {
    const url = new URL(path, this.base_url);
    if (params) {
      for (const [key, value] of Object.entries(params)) {
        if (value !== undefined && value !== '') {
          url.searchParams.set(key, value);
        }
      }
    }

    return this.request<T>(url.toString(), { method: 'GET' });
  }

  async post<T = unknown>(path: string, body?: unknown): Promise<T> {
    const url = new URL(path, this.base_url);
    const options: RequestInit = {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
    };

    if (body !== undefined) {
      options.body = JSON.stringify(body);
    }

    return this.request<T>(url.toString(), options);
  }

  destroy() {
    if (this.health_check_interval) {
      clearInterval(this.health_check_interval);
      this.health_check_interval = null;
    }
  }

  private start_health_monitor() {
    // Periodic health check every 30s to detect plugin restarts
    this.health_check_interval = setInterval(async () => {
      if (this.state === 'disconnected' || this.state === 'reconnecting') {
        await this.check_health();
      }
    }, 30_000);

    // Initial health check
    this.check_health().catch(() => {});
  }

  private async check_health(): Promise<boolean> {
    try {
      const controller = new AbortController();
      const timeout_id = setTimeout(() => controller.abort(), 3000);

      const response = await fetch(`${this.base_url}/api/health`, {
        method: 'GET',
        signal: controller.signal,
      });

      clearTimeout(timeout_id);

      if (response.ok) {
        if (this.state !== 'connected') {
          console.error(`[x64dbg-mcp] Plugin connection established at ${this.base_url}`);
        }
        this.state = 'connected';
        this.consecutive_failures = 0;
        return true;
      }
      return false;
    } catch {
      if (this.state === 'connected') {
        console.error('[x64dbg-mcp] Plugin connection lost, will retry on next request');
      }
      this.state = 'disconnected';
      return false;
    }
  }

  private is_connection_error(err: Error): boolean {
    const msg = err.message.toLowerCase();
    return (
      msg.includes('econnrefused') ||
      msg.includes('econnreset') ||
      msg.includes('epipe') ||
      msg.includes('enotfound') ||
      msg.includes('enetunreach') ||
      msg.includes('ehostunreach') ||
      msg.includes('socket hang up') ||
      msg.includes('fetch failed') ||
      msg.includes('network') ||
      msg.includes('econnaborted') ||
      msg.includes('etimedout')
    );
  }

  private async request<T>(url: string, options: RequestInit): Promise<T> {
    let last_error: Error | null = null;

    // More retries for connection errors (plugin might be restarting)
    const max_retries = this.state === 'connected' ? config.retries : config.retries + 3;

    for (let attempt = 0; attempt <= max_retries; attempt++) {
      try {
        const controller = new AbortController();
        const timeout_id = setTimeout(() => controller.abort(), config.timeout);

        const response = await fetch(url, {
          ...options,
          signal: controller.signal,
        });

        clearTimeout(timeout_id);

        const text = await response.text();
        let parsed: PluginResponse<T>;

        try {
          parsed = JSON.parse(text) as PluginResponse<T>;
        } catch {
          throw new Error(`Invalid JSON response from plugin: ${text.substring(0, 200)}`);
        }

        if (!parsed.success) {
          const err_msg = parsed.error?.message ?? 'Unknown plugin error';
          const err_code = parsed.error?.code ?? 500;
          throw new Error(`Plugin error (${err_code}): ${err_msg}`);
        }

        // Request succeeded - update state
        this.state = 'connected';
        this.consecutive_failures = 0;
        this.last_successful_request = Date.now();

        return parsed.data as T;
      } catch (err) {
        last_error = err instanceof Error ? err : new Error(String(err));

        // Don't retry on client errors (4xx equivalent) - these are real errors
        if (last_error.message.includes('Plugin error (4')) {
          this.state = 'connected'; // Plugin responded, connection is fine
          throw last_error;
        }

        // Don't retry on plugin 409 (conflict / debugger state errors)
        if (last_error.message.includes('Plugin error (409)')) {
          this.state = 'connected';
          throw last_error;
        }

        // Handle timeout
        if (last_error.name === 'AbortError') {
          this.consecutive_failures++;
          if (attempt >= max_retries) {
            throw new Error(
              `Request timed out after ${config.timeout}ms (${attempt + 1} attempts). ` +
              `Is the x64dbg plugin running on ${this.base_url}?`
            );
          }
        }

        // Handle connection errors with exponential backoff
        if (this.is_connection_error(last_error)) {
          this.consecutive_failures++;

          if (this.state === 'connected') {
            this.state = 'reconnecting';
            console.error('[x64dbg-mcp] Connection lost to plugin, attempting reconnect...');
          }

          if (attempt < max_retries) {
            // Exponential backoff: 200ms, 400ms, 800ms, 1600ms, 3200ms
            const delay = Math.min(200 * Math.pow(2, attempt), 5000);
            await new Promise(resolve => setTimeout(resolve, delay));
            continue;
          }

          this.state = 'disconnected';
          throw new Error(
            `Cannot connect to x64dbg plugin at ${this.base_url} after ${attempt + 1} attempts. ` +
            `Make sure x64dbg is running with the MCP plugin loaded. ` +
            `Check with 'mcpserver status' in x64dbg command bar.`
          );
        }

        // Other errors - limited retry with linear backoff
        if (attempt < config.retries) {
          await new Promise(resolve => setTimeout(resolve, 300 * (attempt + 1)));
        }
      }
    }

    throw last_error ?? new Error('Request failed');
  }
}

export const httpClient = new HttpClient();

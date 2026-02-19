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

// How long to wait for the plugin to come online before failing a request
const WAIT_FOR_PLUGIN_TIMEOUT_MS = 120_000; // 2 minutes
const WAIT_POLL_INTERVAL_MS = 2_000;        // poll every 2s

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
    // Periodic health check every 15s to maintain connection awareness
    this.health_check_interval = setInterval(async () => {
      await this.check_health();
    }, 15_000);

    // Initial health check (non-blocking)
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
        const was_disconnected = this.state !== 'connected';
        this.state = 'connected';
        this.consecutive_failures = 0;
        if (was_disconnected) {
          console.error(`[x64dbg-mcp] Plugin connection established at ${this.base_url}`);
        }
        return true;
      }
      return false;
    } catch {
      if (this.state === 'connected') {
        console.error('[x64dbg-mcp] Plugin connection lost, will reconnect automatically');
        this.state = 'disconnected';
      }
      return false;
    }
  }

  /**
   * Block until the plugin is reachable. Polls /api/health every 2s
   * for up to 2 minutes. This is the key fix: instead of failing
   * immediately when the plugin isn't running, we wait for it.
   */
  private async wait_for_connection(): Promise<void> {
    // Already connected - skip
    if (this.state === 'connected') return;

    // Quick check first
    if (await this.check_health()) return;

    // Plugin not available - enter wait loop
    this.state = 'reconnecting';
    console.error(
      `[x64dbg-mcp] Plugin not reachable at ${this.base_url}, waiting up to ${WAIT_FOR_PLUGIN_TIMEOUT_MS / 1000}s for it to come online...`
    );

    const deadline = Date.now() + WAIT_FOR_PLUGIN_TIMEOUT_MS;
    let poll_count = 0;

    while (Date.now() < deadline) {
      await new Promise(resolve => setTimeout(resolve, WAIT_POLL_INTERVAL_MS));
      poll_count++;

      if (await this.check_health()) {
        console.error(`[x64dbg-mcp] Plugin came online after ~${poll_count * 2}s`);
        return;
      }

      // Log progress every 10s
      if (poll_count % 5 === 0) {
        const elapsed = Math.round((Date.now() - (deadline - WAIT_FOR_PLUGIN_TIMEOUT_MS)) / 1000);
        const remaining = Math.round((deadline - Date.now()) / 1000);
        console.error(`[x64dbg-mcp] Still waiting for plugin... (${elapsed}s elapsed, ${remaining}s remaining)`);
      }
    }

    this.state = 'disconnected';
    throw new Error(
      `x64dbg plugin did not come online at ${this.base_url} within ${WAIT_FOR_PLUGIN_TIMEOUT_MS / 1000}s. ` +
      `Make sure x64dbg is running with the MCP plugin loaded. ` +
      `Check with 'mcpserver status' in x64dbg command bar.`
    );
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
    // CRITICAL: If not connected, wait for the plugin to come online
    // This prevents Claude from seeing connection errors and giving up
    await this.wait_for_connection();

    let last_error: Error | null = null;

    for (let attempt = 0; attempt <= config.retries; attempt++) {
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

        // Success
        this.state = 'connected';
        this.consecutive_failures = 0;
        this.last_successful_request = Date.now();

        return parsed.data as T;
      } catch (err) {
        last_error = err instanceof Error ? err : new Error(String(err));

        // Don't retry on 4xx plugin errors - these are valid responses
        if (last_error.message.includes('Plugin error (4')) {
          this.state = 'connected';
          throw last_error;
        }

        // Handle timeout
        if (last_error.name === 'AbortError') {
          this.consecutive_failures++;
          if (attempt >= config.retries) {
            throw new Error(
              `Request timed out after ${config.timeout}ms (${attempt + 1} attempts). ` +
              `The plugin may be busy with a long operation.`
            );
          }
          // Brief pause before retry on timeout
          await new Promise(resolve => setTimeout(resolve, 500));
          continue;
        }

        // Connection errors mid-request: the plugin may have restarted
        if (this.is_connection_error(last_error)) {
          this.consecutive_failures++;
          this.state = 'reconnecting';
          console.error(`[x64dbg-mcp] Connection error on attempt ${attempt + 1}: ${last_error.message}`);

          if (attempt < config.retries) {
            // Wait for reconnection before retrying
            console.error('[x64dbg-mcp] Waiting for plugin to come back...');
            try {
              await this.wait_for_connection();
              continue; // Plugin is back, retry the request
            } catch {
              throw last_error; // Timed out waiting
            }
          }

          this.state = 'disconnected';
          throw new Error(
            `Lost connection to x64dbg plugin at ${this.base_url}. ` +
            `The plugin may have been stopped or x64dbg was closed.`
          );
        }

        // Other errors - brief retry
        if (attempt < config.retries) {
          await new Promise(resolve => setTimeout(resolve, 300 * (attempt + 1)));
        }
      }
    }

    throw last_error ?? new Error('Request failed');
  }
}

export const httpClient = new HttpClient();

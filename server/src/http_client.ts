import { config, getBaseUrl } from './config.js';

interface PluginResponse<T = unknown> {
  success: boolean;
  data?: T;
  error?: {
    code: number;
    message: string;
  };
}

export class HttpClient {
  private baseUrl: string;

  constructor() {
    this.baseUrl = getBaseUrl();
  }

  async get<T = unknown>(path: string, params?: Record<string, string>): Promise<T> {
    const url = new URL(path, this.baseUrl);
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
    const url = new URL(path, this.baseUrl);
    const options: RequestInit = {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
    };

    if (body !== undefined) {
      options.body = JSON.stringify(body);
    }

    return this.request<T>(url.toString(), options);
  }

  private async request<T>(url: string, options: RequestInit): Promise<T> {
    let lastError: Error | null = null;

    for (let attempt = 0; attempt <= config.retries; attempt++) {
      try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), config.timeout);

        const response = await fetch(url, {
          ...options,
          signal: controller.signal,
        });

        clearTimeout(timeoutId);

        const text = await response.text();
        let parsed: PluginResponse<T>;

        try {
          parsed = JSON.parse(text) as PluginResponse<T>;
        } catch {
          throw new Error(`Invalid JSON response from plugin: ${text.substring(0, 200)}`);
        }

        if (!parsed.success) {
          const errMsg = parsed.error?.message ?? 'Unknown plugin error';
          const errCode = parsed.error?.code ?? 500;
          throw new Error(`Plugin error (${errCode}): ${errMsg}`);
        }

        return parsed.data as T;
      } catch (err) {
        lastError = err instanceof Error ? err : new Error(String(err));

        // Don't retry on client errors (4xx equivalent)
        if (lastError.message.includes('Plugin error (4')) {
          throw lastError;
        }

        // Don't retry on abort (timeout)
        if (lastError.name === 'AbortError') {
          throw new Error(`Request to ${url} timed out after ${config.timeout}ms`);
        }

        // Retry on connection errors
        if (attempt < config.retries) {
          await new Promise(resolve => setTimeout(resolve, 200 * (attempt + 1)));
        }
      }
    }

    throw lastError ?? new Error('Request failed');
  }
}

export const httpClient = new HttpClient();

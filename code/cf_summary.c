#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 使用 curl 库
#include <curl/curl.h>
// 使用 cJSON 库
#include "cjson/cJSON.h"

void timestamp_to_date_js(long long timestamp, char *buffer, size_t buffer_size);

#define MAX_URL 512
#define MAX_BUFFER 1048576

typedef struct {
    char *data;
    size_t size;
} StringBuffer;

typedef struct {
    char handle[128];
    char rank[128];
    char titlePhoto[512];
    int rating;
    int max_rating;
    int contest_count;
    int recent_180_count;
    int recent_180_max;
} UserStats;

// curl 回调函数
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    StringBuffer *sb = (StringBuffer *)userp;
    size_t realsize = size * nmemb;
    sb->data = realloc(sb->data, sb->size + realsize + 1);
    memcpy(&sb->data[sb->size], contents, realsize);
    sb->size += realsize;
    sb->data[sb->size] = '\0';
    return realsize;
}

// 使用 curl 获取 URL 内容
char *fetch_url(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    StringBuffer sb;
    sb.data = malloc(1);
    sb.size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sb);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(sb.data);
        return NULL;
    }

    return sb.data;
}

void parse_user_info(const char *json_data, UserStats *stats) {
    cJSON *root = cJSON_Parse(json_data);
    if (!root) return;

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if (!result || !cJSON_IsArray(result)) {
        cJSON_Delete(root);
        return;
    }

    cJSON *user = cJSON_GetArrayItem(result, 0);
    if (!user) {
        cJSON_Delete(root);
        return;
    }

    cJSON *handle = cJSON_GetObjectItem(user, "handle");
    cJSON *rank = cJSON_GetObjectItem(user, "rank");
    cJSON *titlePhoto = cJSON_GetObjectItem(user, "titlePhoto");
    cJSON *rating = cJSON_GetObjectItem(user, "rating");
    cJSON *max_rating = cJSON_GetObjectItem(user, "maxRating");

    if (handle && handle->valuestring) strcpy(stats->handle, handle->valuestring);
    if (rank && rank->valuestring) strcpy(stats->rank, rank->valuestring);
    if (titlePhoto && titlePhoto->valuestring) strcpy(stats->titlePhoto, titlePhoto->valuestring);
    if (rating) stats->rating = rating->valueint;
    if (max_rating) stats->max_rating = max_rating->valueint;

    cJSON_Delete(root);
}

void timestamp_to_date(long long timestamp, char *buffer, size_t buffer_size) {
    time_t t = (time_t)timestamp;
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        strftime(buffer, buffer_size, "%Y-%m-%d", tm_info);
    }
}

void get_rating_color(int rating, char *color) {
    if (rating < 1200) strcpy(color, "#808080");
    else if (rating < 1400) strcpy(color, "#008000");
    else if (rating < 1600) strcpy(color, "#00CED1");
    else if (rating < 1900) strcpy(color, "#0000FF");
    else if (rating < 2200) strcpy(color, "#800080");
    else if (rating < 2400) strcpy(color, "#FF8C00");
    else strcpy(color, "#FF0000");
}

void generate_user_summary(const char *handle, FILE *out) {
    fprintf(out, "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n");
    fprintf(out, "    <meta charset=\"UTF-8\">\n");
    fprintf(out, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(out, "    <title>%s - Codeforces Contest Summary</title>\n", handle);
    
    // 引入 eChart
    fprintf(out, "    <script>\n");
    fprintf(out, "        window.echartsLoaded = false;\n");
    fprintf(out, "        function checkECharts() {\n");
    fprintf(out, "            if (typeof echarts !== 'undefined') {\n");
    fprintf(out, "                window.echartsLoaded = true;\n");
    fprintf(out, "                if (typeof initCharts === 'function') {\n");
    fprintf(out, "                    initCharts();\n");
    fprintf(out, "                }\n");
    fprintf(out, "            }\n");
    fprintf(out, "        }\n");
    fprintf(out, "    </script>\n");
    fprintf(out, "    <script src=\"lib/echarts/echarts.min.js\" onload=\"checkECharts()\" onerror=\"console.log('ECharts loading failed')\"></script>\n");
    
    fprintf(out, "    <style>\n");
    fprintf(out, "        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }\n");
    fprintf(out, "        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }\n");
    fprintf(out, "        .user-card { display: flex; align-items: center; gap: 20px; margin-bottom: 30px; padding: 20px; background: #f8f9fa; border-radius: 10px; }\n");
    fprintf(out, "        .avatar { width: 100px; height: 100px; border-radius: 50%%; }\n");
    fprintf(out, "        h1 { color: #333; }\n");
    fprintf(out, "        h2 { margin: 0; }\n");
    fprintf(out, "        h3 { color: #555; border-bottom: 2px solid #007bff; padding-bottom: 10px; }\n");
    fprintf(out, "        .contest-stats ul { list-style: none; padding: 0; }\n");
    fprintf(out, "        .contest-stats li { padding: 8px 0; border-bottom: 1px solid #eee; }\n");
    fprintf(out, "        table { width: 100%%; border-collapse: collapse; margin-top: 20px; font-size: 12px; }\n");
    fprintf(out, "        th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }\n");
    fprintf(out, "        th { background: #5c7cfa; color: white; font-weight: bold; }\n");
    fprintf(out, "        tr:nth-child(even) { background: #f8fafc; }\n");
    fprintf(out, "        tr:nth-child(odd) { background: #ffffff; }\n");
    fprintf(out, "        tr:hover { background: #e0e7ff; }\n");
    fprintf(out, "        .positive { color: green; font-weight: bold; }\n");
    fprintf(out, "        .negative { color: red; font-weight: bold; }\n");
    fprintf(out, "        .chart-section { margin-top: 30px; }\n");
    fprintf(out, "        #trendChart, #histChart { width: 100%%; height: 400px; }\n");
    fprintf(out, "        .back-link { display: inline-block; margin-bottom: 20px; padding: 10px 20px; background: #007bff; color: white; text-decoration: none; border-radius: 5px; }\n");
    fprintf(out, "        .back-link:hover { background: #0056b3; }\n");
    fprintf(out, "        .filter-buttons { margin-bottom: 15px; }\n");
    fprintf(out, "        .filter-btn { padding: 6px 12px; margin-right: 8px; border: 1px solid #ddd; background: white; border-radius: 4px; cursor: pointer; }\n");
    fprintf(out, "        .filter-btn.active { background: #5c7cfa; color: white; border-color: #5c7cfa; }\n");
    fprintf(out, "    </style>\n");
    fprintf(out, "</head>\n");
    fprintf(out, "<body>\n");
    fprintf(out, "<div class=\"container\">\n");
    fprintf(out, "    <a href=\"index.html\" class=\"back-link\">← 返回用户列表</a>\n");

    // 获取用户信息
    char url[MAX_URL];
    snprintf(url, sizeof(url), "https://codeforces.com/api/user.info?handles=%s", handle);
    char *user_data = fetch_url(url);
    
    UserStats stats = {0};
    if (user_data) {
        parse_user_info(user_data, &stats);
        free(user_data);
    }

    // 用户卡片
    fprintf(out, "    <div class=\"user-card\">\n");
    fprintf(out, "        <img src=\"%s\" alt=\"Avatar\" class=\"avatar\">\n", stats.titlePhoto);
    fprintf(out, "        <div>\n");
    
    char handle_color[16];
    get_rating_color(stats.rating, handle_color);
    fprintf(out, "            <h1 style=\"color: %s;\">%s</h1>\n", handle_color, stats.handle);
    fprintf(out, "            <h2>%s</h2>\n", stats.rank);
    fprintf(out, "            <p>当前等级分: <strong style=\"color: %s;\">%d</strong></p>\n", handle_color, stats.rating);
    fprintf(out, "        </div>\n");
    fprintf(out, "    </div>\n");

    // 等级分趋势折线图
    fprintf(out, "    <div class=\"chart-section\">\n");
    fprintf(out, "        <h3>Rating Trend</h3>\n");
    fprintf(out, "        <div id=\"trendChart\"></div>\n");
    fprintf(out, "    </div>\n");

    // 获取比赛历史
    snprintf(url, sizeof(url), "https://codeforces.com/api/user.rating?handle=%s", handle);
    char *result = fetch_url(url);
    
    // 存储图表数据（使用动态分配）
    char *dates_buffer = malloc(65536);
    char *ratings_buffer = malloc(65536);
    if (dates_buffer) memset(dates_buffer, 0, 65536);
    if (ratings_buffer) memset(ratings_buffer, 0, 65536);
    
    // 存储比赛信息用于补题统计
    int max_contests = 500;
    int *contest_ids = malloc(max_contests * sizeof(int));
    long long *contest_times = malloc(max_contests * sizeof(long long));
    char **contest_names = malloc(max_contests * sizeof(char*));
    int contest_count = 0;
    
    // 比赛ID到索引的映射（Codeforces比赛ID已经超过100000）
    int *contest_id_to_idx = calloc(300000, sizeof(int));
    for (int i = 0; i < 300000; i++) {
        contest_id_to_idx[i] = -1;
    }
    
    if (result) {
        cJSON *root = cJSON_Parse(result);
        if (root) {
            cJSON *result_array = cJSON_GetObjectItem(root, "result");
            if (result_array && cJSON_IsArray(result_array)) {
                int total_contests = cJSON_GetArraySize(result_array);
                
                // 统计数据
                int max_rating = 0;
                long long now = time(NULL);
                long long days_180 = 180LL * 24 * 3600;
                int recent_180_count = 0;
                int recent_180_max = 0;

                // 先收集所有比赛信息
                for (int i = 0; i < total_contests && i < max_contests; i++) {
                    cJSON *contest = cJSON_GetArrayItem(result_array, i);
                    if (!contest) continue;
                    
                    cJSON *contest_id = cJSON_GetObjectItem(contest, "contestId");
                    cJSON *timestamp = cJSON_GetObjectItem(contest, "ratingUpdateTimeSeconds");
                    cJSON *contest_name = cJSON_GetObjectItem(contest, "contestName");
                    
                    if (contest_id && timestamp) {
                        contest_ids[contest_count] = contest_id->valueint;
                        contest_times[contest_count] = timestamp->valueint;
                        contest_id_to_idx[contest_id->valueint] = contest_count;
                        if (contest_name && contest_name->valuestring) {
                            contest_names[contest_count] = strdup(contest_name->valuestring);
                        } else {
                            contest_names[contest_count] = strdup("Unknown");
                        }
                        contest_count++;
                    }
                }
                
                // 比赛历史表格
                fprintf(out, "    <div class=\"contest-history\">\n");
                fprintf(out, "        <h3>Contest History</h3>\n");
                fprintf(out, "        <table>\n");
                fprintf(out, "            <tr><th>Contest</th><th>Date</th><th>Rank</th><th>Old Rating</th><th>New Rating</th><th>Change</th><th>Problem Details</th><th>Solved</th><th>Post-contest</th></tr>\n");

                for (int i = total_contests - 1; i >= 0; i--) {
                    cJSON *contest = cJSON_GetArrayItem(result_array, i);
                    if (!contest) continue;

                    cJSON *contest_name = cJSON_GetObjectItem(contest, "contestName");
                    cJSON *rank = cJSON_GetObjectItem(contest, "rank");
                    cJSON *old_rating = cJSON_GetObjectItem(contest, "oldRating");
                    cJSON *new_rating = cJSON_GetObjectItem(contest, "newRating");
                    cJSON *timestamp = cJSON_GetObjectItem(contest, "ratingUpdateTimeSeconds");
                    cJSON *contest_id = cJSON_GetObjectItem(contest, "contestId");

                    if (contest_name && contest_name->valuestring && rank && old_rating && new_rating) {
                        char date_str[32];
                        if (timestamp) {
                            timestamp_to_date_js(timestamp->valueint, date_str, sizeof(date_str));
                        } else {
                            strcpy(date_str, "Unknown");
                        }

                        int delta = new_rating->valueint - old_rating->valueint;
                        
                        // 统计
                        if (new_rating->valueint > max_rating) max_rating = new_rating->valueint;
                        
                        if (timestamp && (now - timestamp->valueint) <= days_180) {
                            recent_180_count++;
                            if (new_rating->valueint > recent_180_max) recent_180_max = new_rating->valueint;
                        }

                        // 存储图表数据
                        if (dates_buffer && ratings_buffer) {
                            char temp[256];
                            snprintf(temp, sizeof(temp), "        dates.push('%s');\n", date_str);
                            strcat(dates_buffer, temp);
                            snprintf(temp, sizeof(temp), "        ratings.push(%d);\n", new_rating->valueint);
                            strcat(ratings_buffer, temp);
                        }

                        // 表格行
                        char old_color[16], new_color[16];
                        get_rating_color(old_rating->valueint, old_color);
                        get_rating_color(new_rating->valueint, new_color);
                        
                        fprintf(out, "            <tr>");
                        fprintf(out, "<td>%s</td>", contest_name->valuestring);
                        fprintf(out, "<td>%s</td>", date_str);
                        fprintf(out, "<td>%d</td>", rank->valueint);
                        fprintf(out, "<td style=\"color: %s;\">%d</td>", old_color, old_rating->valueint);
                        fprintf(out, "<td style=\"color: %s;\">%d</td>", new_color, new_rating->valueint);
                        fprintf(out, "<td %s>%+d</td>", delta >= 0 ? "class=\"positive\"" : "class=\"negative\"", delta);
                        fprintf(out, "<td id=\"problems_%d\">-</td>", contest_id ? contest_id->valueint : 0);
                        fprintf(out, "<td id=\"solved_%d\">-</td>", contest_id ? contest_id->valueint : 0);
                        fprintf(out, "<td id=\"upsolve_%d\">-</td>", contest_id ? contest_id->valueint : 0);
                        fprintf(out, "</tr>\n");
                    }
                }

                fprintf(out, "        </table>\n");
                fprintf(out, "    </div>\n");

                stats.contest_count = total_contests;
                stats.max_rating = max_rating;
                stats.recent_180_count = recent_180_count;
                stats.recent_180_max = recent_180_max;
                
                // 生成图表数据的 JavaScript
                fprintf(out, "    <script>\n");
                fprintf(out, "        function initCharts() {\n");
                fprintf(out, "            if (typeof echarts === 'undefined') return;\n");
                fprintf(out, "            var trendChart = echarts.init(document.getElementById('trendChart'));\n");
                fprintf(out, "        var dates = [];\n");
                fprintf(out, "        var ratings = [];\n");
                if (dates_buffer) fprintf(out, "%s", dates_buffer);
                if (ratings_buffer) fprintf(out, "%s", ratings_buffer);

                // 图表配置
                fprintf(out, "        var option = {\n");
                fprintf(out, "            tooltip: { trigger: 'axis' },\n");
                fprintf(out, "            grid: { left: '3%%', right: '4%%', bottom: '15%%', containLabel: true },\n");
                fprintf(out, "            xAxis: {\n");
                fprintf(out, "                type: 'category',\n");
                fprintf(out, "                data: dates,\n");
                fprintf(out, "                axisLabel: { rotate: 45, fontSize: 10 }\n");
                fprintf(out, "            },\n");
                fprintf(out, "            yAxis: {\n");
                fprintf(out, "                type: 'value',\n");
                fprintf(out, "                name: 'Rating',\n");
                fprintf(out, "                min: function(value) { return Math.floor(value.min / 100) * 100 - 100; },\n");
                fprintf(out, "                max: function(value) { return Math.ceil(value.max / 100) * 100 + 100; }\n");
                fprintf(out, "            },\n");
                fprintf(out, "            series: [{\n");
                fprintf(out, "                name: 'Rating',\n");
                fprintf(out, "                type: 'line',\n");
                fprintf(out, "                data: ratings,\n");
                fprintf(out, "                smooth: true,\n");
                fprintf(out, "                lineStyle: { color: '#5470c6', width: 2 },\n");
                fprintf(out, "                areaStyle: {\n");
                fprintf(out, "                    color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [\n");
                fprintf(out, "                        { offset: 0, color: 'rgba(84, 112, 198, 0.5)' },\n");
                fprintf(out, "                        { offset: 1, color: 'rgba(84, 112, 198, 0.1)' }\n");
                fprintf(out, "                    ])\n");
                fprintf(out, "                },\n");
                fprintf(out, "                symbol: 'circle',\n");
                fprintf(out, "                symbolSize: 4\n");
                fprintf(out, "            }]\n");
                fprintf(out, "        };\n");
                fprintf(out, "            trendChart.setOption(option);\n");
                fprintf(out, "        }\n");
                fprintf(out, "        \n");
                fprintf(out, "        // 如果ECharts已经加载，立即初始化图表\n");
                fprintf(out, "        if (window.echartsLoaded && typeof echarts !== 'undefined') {\n");
                fprintf(out, "            initCharts();\n");
                fprintf(out, "        }\n");
                fprintf(out, "        \n");
                fprintf(out, "        // 页面加载时尝试初始化图表\n");
                fprintf(out, "        window.addEventListener('load', function() {\n");
                fprintf(out, "            setTimeout(checkECharts, 100);\n");
                fprintf(out, "        });\n");
                fprintf(out, "    </script>\n");
            }
            cJSON_Delete(root);
        }
        free(result);
    }
    
    // 释放内存
    if (dates_buffer) free(dates_buffer);
    if (ratings_buffer) free(ratings_buffer);

    // 统计信息
    fprintf(out, "    <div class=\"contest-stats\">\n");
    fprintf(out, "        <h3>Statistics</h3>\n");
    fprintf(out, "        <ul>\n");
    fprintf(out, "            <li>比赛次数: %d</li>\n", stats.contest_count);
    fprintf(out, "            <li>最高等级分: %d</li>\n", stats.max_rating);
    fprintf(out, "            <li>近180天比赛次数: %d</li>\n", stats.recent_180_count);
    fprintf(out, "            <li>近180天最高等级分: %d</li>\n", stats.recent_180_max);
    fprintf(out, "        </ul>\n");

    fprintf(out, "    </div>\n");

    // 题目难度分布直方图
    fprintf(out, "    <div class=\"chart-section\">\n");
    fprintf(out, "        <h3>Problem Rating Distribution</h3>\n");
    fprintf(out, "        <div class=\"filter-buttons\">\n");
    fprintf(out, "            <button class=\"filter-btn active\" onclick=\"showHistogram('all')\">All</button>\n");
    fprintf(out, "            <button class=\"filter-btn\" onclick=\"showHistogram('year')\">Last Year</button>\n");
    fprintf(out, "            <button class=\"filter-btn\" onclick=\"showHistogram('180d')\">Last 180 Days</button>\n");
    fprintf(out, "            <button class=\"filter-btn\" onclick=\"showHistogram('month')\">Last Month</button>\n");
    fprintf(out, "        </div>\n");
    fprintf(out, "        <div id=\"histChart\"></div>\n");
    fprintf(out, "    </div>\n");

    // 统计各难度区间的唯一通过题目数
    int all_counts[9] = {0};
    int year_counts[9] = {0};
    int day180_counts[9] = {0};
    int month_counts[9] = {0};
    
    // 使用哈希表来跟踪已通过的题目（按contestId+index唯一标识）
    int max_problems = 10000;
    long long *all_problems = calloc(max_problems, sizeof(long long));
    int *all_problem_times = calloc(max_problems, sizeof(int));
    int all_problem_count = 0;
    
    // 补题统计：记录每个比赛的补题数和题目详情
    int *upsolve_counts = calloc(max_contests, sizeof(int));
    
    // 存储每个比赛的题目详情
    char **in_contest_problems = calloc(max_contests, sizeof(char*));
    char **upsolve_problems = calloc(max_contests, sizeof(char*));
    int *solved_counts = calloc(max_contests, sizeof(int));
    
    for (int i = 0; i < max_contests; i++) {
        in_contest_problems[i] = calloc(500, sizeof(char));
        upsolve_problems[i] = calloc(500, sizeof(char));
        solved_counts[i] = 0;
    }
    
    long long now = time(NULL);
    long long year_ago = now - 365LL * 24 * 3600;
    long long day180_ago = now - 180LL * 24 * 3600;
    long long month_ago = now - 30LL * 24 * 3600;
    
    // 分批获取提交记录（每次最多1000条，最多获取8000条）
    int max_requests = 8;
    int total_fetched = 0;
    
    for (int req = 0; req < max_requests; req++) {
        int from = req * 1000 + 1;
        snprintf(url, sizeof(url), "https://codeforces.com/api/user.status?handle=%s&from=%d&count=1000", handle, from);
        char *submissions = fetch_url(url);
        
        if (!submissions) {
            break;
        }
        
        printf("Fetched batch %d from %d, size: %d bytes\n", req+1, from, (int)strlen(submissions));
        
        cJSON *root = cJSON_Parse(submissions);
        if (!root) {
            free(submissions);
            break;
        }
        
        cJSON *result_array = cJSON_GetObjectItem(root, "result");
        if (!result_array || !cJSON_IsArray(result_array)) {
            cJSON_Delete(root);
            free(submissions);
            break;
        }
        
        int batch_size = cJSON_GetArraySize(result_array);
        if (batch_size == 0) {
            cJSON_Delete(root);
            free(submissions);
            break;
        }
        
        total_fetched += batch_size;
        
        for (int i = 0; i < batch_size; i++) {
            cJSON *sub = cJSON_GetArrayItem(result_array, i);
            if (!sub) continue;
            
            cJSON *verdict = cJSON_GetObjectItem(sub, "verdict");
            cJSON *problem = cJSON_GetObjectItem(sub, "problem");
            cJSON *time = cJSON_GetObjectItem(sub, "creationTimeSeconds");
            
            if (!verdict || !verdict->valuestring || strcmp(verdict->valuestring, "OK") != 0) continue;
            if (!problem) continue;
            
            cJSON *rating = cJSON_GetObjectItem(problem, "rating");
            cJSON *contest_id = cJSON_GetObjectItem(problem, "contestId");
            cJSON *index = cJSON_GetObjectItem(problem, "index");
            
            if (!contest_id || !index) continue;
            
            // 创建唯一标识符：使用完整的index字符串
            long long problem_id = ((long long)contest_id->valueint << 32);
            for (int j = 0; index->valuestring[j] && j < 4; j++) {
                problem_id = (problem_id << 8) | (index->valuestring[j] & 0xFF);
            }
            
            // 检查是否已经统计过这道题
            int found = 0;
            int earliest_time = time ? time->valueint : 0;
            
            for (int p = 0; p < all_problem_count; p++) {
                if (all_problems[p] == problem_id) {
                    found = 1;
                    if (earliest_time < all_problem_times[p]) {
                        all_problem_times[p] = earliest_time;
                    }
                    break;
                }
            }
            
            // 检查是否是补题（提交时间晚于比赛时间）
            int submission_time = time ? time->valueint : 0;
            int is_upsolve = 0;
            int contest_idx = contest_id_to_idx[contest_id->valueint];
            
            // 如果比赛不在rating历史中，动态添加
            if (contest_idx < 0 && contest_count < max_contests) {
                contest_ids[contest_count] = contest_id->valueint;
                contest_times[contest_count] = submission_time; // 使用提交时间作为比赛时间
                contest_id_to_idx[contest_id->valueint] = contest_count;
                contest_names[contest_count] = strdup("Unknown");
                contest_idx = contest_count;
                contest_count++;
            }
            
            if (contest_idx >= 0 && submission_time > contest_times[contest_idx]) {
                is_upsolve = 1;
            }
            
            // 如果是新题目，添加到列表并统计
            if (!found && all_problem_count < max_problems) {
                all_problems[all_problem_count] = problem_id;
                all_problem_times[all_problem_count] = earliest_time;
                all_problem_count++;
            }
            
            // 添加题目详情到相应的数组（每个比赛单独记录）
            if (contest_idx >= 0) {
                // 检查题目是否已经在该比赛中记录过
                if (is_upsolve) {
                    // 补题：添加到upsolve_problems
                    if (strstr(upsolve_problems[contest_idx], index->valuestring) == NULL) {
                        if (strlen(upsolve_problems[contest_idx]) > 0) {
                            strcat(upsolve_problems[contest_idx], ", ");
                        }
                        strcat(upsolve_problems[contest_idx], index->valuestring);
                        upsolve_counts[contest_idx]++;
                    }
                } else {
                    // 赛中通过：添加到in_contest_problems
                    if (strstr(in_contest_problems[contest_idx], index->valuestring) == NULL) {
                        if (strlen(in_contest_problems[contest_idx]) > 0) {
                            strcat(in_contest_problems[contest_idx], ", ");
                        }
                        strcat(in_contest_problems[contest_idx], index->valuestring);
                        solved_counts[contest_idx]++;
                    }
                }
            }
            
            if (rating) {
                int r = rating->valueint;
                int idx;
                if (r < 1200) idx = 0;
                else if (r < 1400) idx = 1;
                else if (r < 1600) idx = 2;
                else if (r < 1900) idx = 3;
                else if (r < 2200) idx = 4;
                else if (r < 2400) idx = 5;
                else if (r < 2600) idx = 6;
                else if (r < 3000) idx = 7;
                else idx = 8;
                
                all_counts[idx]++;
                
                if (earliest_time >= year_ago) year_counts[idx]++;
                if (earliest_time >= day180_ago) day180_counts[idx]++;
                if (earliest_time >= month_ago) month_counts[idx]++;
            }
        }
        
        cJSON_Delete(root);
        free(submissions);
        
        if (batch_size < 1000) {
            break;
        }
    }
    
    // 释放内存
    free(all_problems);
    free(all_problem_times);
                
    // Generate contest data JavaScript
    fprintf(out, "    <script>\n");
    fprintf(out, "        // Contest problem data\n");
    fprintf(out, "        var contestData = {\n");
    for (int c = 0; c < contest_count; c++) {
        fprintf(out, "            %d: { inContest: '%s', upsolve: '%s', solved: %d, total: 0 },\n", 
                contest_ids[c], in_contest_problems[c], upsolve_problems[c], solved_counts[c]);
    }
    fprintf(out, "        };\n");
    fprintf(out, "\n");
    fprintf(out, "        // Update table data\n");
    fprintf(out, "        function updateTableData() {\n");
    fprintf(out, "            for (var contestId in contestData) {\n");
    fprintf(out, "                var data = contestData[contestId];\n");
    fprintf(out, "                \n");
    fprintf(out, "                // Update problem details column\n");
    fprintf(out, "                var problemsElem = document.getElementById('problems_' + contestId);\n");
    fprintf(out, "                if (problemsElem) {\n");
    fprintf(out, "                    var details = data.inContest;\n");
    fprintf(out, "                    if (data.upsolve.length > 0) {\n");
    fprintf(out, "                        details += ' (+' + data.upsolve + ')';\n");
    fprintf(out, "                    }\n");
    fprintf(out, "                    problemsElem.innerHTML = details || '-';\n");
    fprintf(out, "                }\n");
    fprintf(out, "                \n");
    fprintf(out, "                // Update Solved column\n");
    fprintf(out, "                var solvedElem = document.getElementById('solved_' + contestId);\n");
    fprintf(out, "                if (solvedElem) {\n");
    fprintf(out, "                    solvedElem.innerHTML = data.solved;\n");
    fprintf(out, "                }\n");
    fprintf(out, "                \n");
    fprintf(out, "                // Update Post-contest column\n");
    fprintf(out, "                var upsolveElem = document.getElementById('upsolve_' + contestId);\n");
    fprintf(out, "                if (upsolveElem) {\n");
    fprintf(out, "                    var upsolveCount = data.upsolve.length > 0 ? data.upsolve.split(',').length : 0;\n");
    fprintf(out, "                    upsolveElem.innerHTML = upsolveCount;\n");
    fprintf(out, "                    if (upsolveCount > 0) {\n");
    fprintf(out, "                        upsolveElem.style.color = '#008000';\n");
    fprintf(out, "                        upsolveElem.style.fontWeight = 'bold';\n");
    fprintf(out, "                    }\n");
    fprintf(out, "                }\n");
    fprintf(out, "            }\n");
    fprintf(out, "        }\n");
    fprintf(out, "\n");
    fprintf(out, "        // Update table when page loads\n");
    fprintf(out, "        document.addEventListener('DOMContentLoaded', function() {\n");
    fprintf(out, "            updateTableData();\n");
    fprintf(out, "        });\n");
    fprintf(out, "        // Fallback for older browsers\n");
    fprintf(out, "        window.onload = function() {\n");
    fprintf(out, "            updateTableData();\n");
    fprintf(out, "        };\n");
    fprintf(out, "    </script>\n");
    
    // 释放内存
    for (int i = 0; i < max_contests; i++) {
        free(in_contest_problems[i]);
        free(upsolve_problems[i]);
    }
    free(in_contest_problems);
    free(upsolve_problems);
    free(solved_counts);
    free(upsolve_counts);
    
    // 生成直方图 JavaScript
    fprintf(out, "    <script>\n");
    fprintf(out, "        var histChart = null;\n");
    fprintf(out, "        var categories = ['<1200', '1200-1399', '1400-1599', '1600-1899', '1900-2199', '2200-2399', '2400-2599', '2600-2999', '>=3000'];\n");
    fprintf(out, "        var allData = [%d, %d, %d, %d, %d, %d, %d, %d, %d];\n", 
            all_counts[0], all_counts[1], all_counts[2], all_counts[3], all_counts[4], 
            all_counts[5], all_counts[6], all_counts[7], all_counts[8]);
    fprintf(out, "        var yearData = [%d, %d, %d, %d, %d, %d, %d, %d, %d];\n", 
            year_counts[0], year_counts[1], year_counts[2], year_counts[3], year_counts[4], 
            year_counts[5], year_counts[6], year_counts[7], year_counts[8]);
    fprintf(out, "        var day180Data = [%d, %d, %d, %d, %d, %d, %d, %d, %d];\n", 
            day180_counts[0], day180_counts[1], day180_counts[2], day180_counts[3], day180_counts[4], 
            day180_counts[5], day180_counts[6], day180_counts[7], day180_counts[8]);
    fprintf(out, "        var monthData = [%d, %d, %d, %d, %d, %d, %d, %d, %d];\n", 
            month_counts[0], month_counts[1], month_counts[2], month_counts[3], month_counts[4], 
            month_counts[5], month_counts[6], month_counts[7], month_counts[8]);
    
    fprintf(out, "        var colors = ['#808080', '#008000', '#00CED1', '#0000FF', '#800080', '#FF8C00', '#FF0000', '#FF0000', '#FF0000'];\n");
    
    fprintf(out, "        function initHistogram() {\n");
    fprintf(out, "            if (typeof echarts === 'undefined') return;\n");
    fprintf(out, "            histChart = echarts.init(document.getElementById('histChart'));\n");
    fprintf(out, "            setHistogramOption(allData);\n");
    fprintf(out, "        }\n");
    
    fprintf(out, "        function setHistogramOption(data) {\n");
    fprintf(out, "            if (!histChart) return;\n");
    fprintf(out, "            var option = {\n");
    fprintf(out, "                tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' } },\n");
    fprintf(out, "                grid: { left: '3%%', right: '4%%', bottom: '3%%', containLabel: true },\n");
    fprintf(out, "                xAxis: { type: 'category', data: categories, axisLabel: { rotate: 30, fontSize: 10 } },\n");
    fprintf(out, "                yAxis: { type: 'value', name: 'Count' },\n");
    fprintf(out, "                series: [{\n");
    fprintf(out, "                    type: 'bar',\n");
    fprintf(out, "                    data: data.map(function(val, idx) { return { value: val, itemStyle: { color: colors[idx] } }; }),\n");
    fprintf(out, "                    barWidth: '60%%'\n");
    fprintf(out, "                }]\n");
    fprintf(out, "            };\n");
    fprintf(out, "            histChart.setOption(option);\n");
    fprintf(out, "        }\n");
    
    fprintf(out, "        function showHistogram(type) {\n");
    fprintf(out, "            if (!histChart) return;\n");
    fprintf(out, "            document.querySelectorAll('.filter-btn').forEach(function(btn) { btn.classList.remove('active'); });\n");
    fprintf(out, "            event.target.classList.add('active');\n");
    fprintf(out, "            switch(type) {\n");
    fprintf(out, "                case 'all': setHistogramOption(allData); break;\n");
    fprintf(out, "                case 'year': setHistogramOption(yearData); break;\n");
    fprintf(out, "                case '180d': setHistogramOption(day180Data); break;\n");
    fprintf(out, "                case 'month': setHistogramOption(monthData); break;\n");
    fprintf(out, "            }\n");
    fprintf(out, "        }\n");
    fprintf(out, "        \n");
    fprintf(out, "        // 将直方图初始化添加到 initCharts 函数（如果存在）\n");
    fprintf(out, "        if (typeof initCharts === 'function') {\n");
    fprintf(out, "            var originalInitCharts = initCharts;\n");
    fprintf(out, "            initCharts = function() {\n");
    fprintf(out, "                originalInitCharts();\n");
    fprintf(out, "                initHistogram();\n");
    fprintf(out, "            };\n");
    fprintf(out, "        } else {\n");
    fprintf(out, "            // 如果 initCharts 不存在，直接在页面加载时初始化直方图\n");
    fprintf(out, "            window.addEventListener('load', function() {\n");
    fprintf(out, "                initHistogram();\n");
    fprintf(out, "            });\n");
    fprintf(out, "        }\n");
    fprintf(out, "    </script>\n");

    fprintf(out, "</div>\n");
    fprintf(out, "</body>\n");
    fprintf(out, "</html>\n");
}

void capitalize_string(char *str) {
    if (str && strlen(str) > 0) {
        str[0] = toupper(str[0]);
        for (int i = 1; str[i]; i++) {
            if (str[i-1] == ' ') {
                str[i] = toupper(str[i]);
            }
        }
    }
}

void generate_users_index(const char *users_file, FILE *out) {
    fprintf(out, "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n");
    fprintf(out, "    <meta charset=\"UTF-8\">\n");
    fprintf(out, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(out, "    <title>Codeforces 用户列表</title>\n");
    fprintf(out, "    <style>\n");
    fprintf(out, "        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }\n");
    fprintf(out, "        .container { max-width: 1400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }\n");
    fprintf(out, "        h1 { color: #333; text-align: center; }\n");
    fprintf(out, "        .users-table { width: 100%%; border-collapse: collapse; margin-top: 20px; }\n");
    fprintf(out, "        .users-table th, .users-table td { padding: 12px; text-align: center; border-bottom: 1px solid #ddd; }\n");
    fprintf(out, "        .users-table th { background: #5c7cfa; color: white; }\n");
    fprintf(out, "        .users-table tr:hover { background: #f8fafc; }\n");
    fprintf(out, "        .users-table a { text-decoration: none; font-weight: bold; }\n");
    fprintf(out, "        .legend { margin-top: 20px; padding: 15px; background: #f8fafc; border-radius: 5px; }\n");
    fprintf(out, "        .legend h3 { margin-top: 0; }\n");
    fprintf(out, "        .legend-item { display: inline-block; margin-right: 20px; }\n");
    fprintf(out, "        .legend-color { display: inline-block; width: 20px; height: 20px; border-radius: 50%%; vertical-align: middle; margin-right: 5px; }\n");
    fprintf(out, "    </style>\n");
    fprintf(out, "</head>\n");
    fprintf(out, "<body>\n");
    fprintf(out, "<div class=\"container\">\n");
    fprintf(out, "    <h1>Codeforces 用户列表</h1>\n");
    fprintf(out, "    <table class=\"users-table\">\n");
    fprintf(out, "        <tr><th>用户ID</th><th>头衔</th><th>当前等级分</th><th>比赛次数</th><th>最高等级分</th><th>近180天比赛次数</th><th>近180天最高等级分</th></tr>\n");

    FILE *f = fopen(users_file, "r");
    if (f) {
        char handle[128];
        while (fgets(handle, sizeof(handle), f)) {
            handle[strcspn(handle, "\n")] = 0;
            if (strlen(handle) == 0) continue;

            char url[MAX_URL];
            snprintf(url, sizeof(url), "https://codeforces.com/api/user.info?handles=%s", handle);
            char *json_data = fetch_url(url);

            char rank_str[128] = "Unknown";
            int rating_val = 0, max_val = 0;

            if (json_data) {
                cJSON *root = cJSON_Parse(json_data);
                if (root) {
                    cJSON *result = cJSON_GetObjectItem(root, "result");
                    if (result && cJSON_IsArray(result)) {
                        cJSON *user = cJSON_GetArrayItem(result, 0);
                        if (user) {
                            cJSON *rank = cJSON_GetObjectItem(user, "rank");
                            cJSON *rating = cJSON_GetObjectItem(user, "rating");
                            cJSON *max_rating = cJSON_GetObjectItem(user, "maxRating");

                            if (rank && rank->valuestring) {
                                strcpy(rank_str, rank->valuestring);
                                capitalize_string(rank_str);
                            }
                            if (rating) rating_val = rating->valueint;
                            if (max_rating) max_val = max_rating->valueint;
                        }
                    }
                    cJSON_Delete(root);
                }
                free(json_data);
            }

            // 获取比赛历史
            int contest_count = 0;
            int recent_180_count = 0;
            int recent_180_max = 0;
            
            snprintf(url, sizeof(url), "https://codeforces.com/api/user.rating?handle=%s", handle);
            char *rating_data = fetch_url(url);
            
            if (rating_data) {
                cJSON *root = cJSON_Parse(rating_data);
                if (root) {
                    cJSON *result_array = cJSON_GetObjectItem(root, "result");
                    if (result_array && cJSON_IsArray(result_array)) {
                        contest_count = cJSON_GetArraySize(result_array);
                        
                        long long now = time(NULL);
                        long long days_180 = 180LL * 24 * 3600;
                        
                        for (int i = 0; i < contest_count; i++) {
                            cJSON *contest = cJSON_GetArrayItem(result_array, i);
                            if (contest) {
                                cJSON *new_rating = cJSON_GetObjectItem(contest, "newRating");
                                cJSON *timestamp = cJSON_GetObjectItem(contest, "ratingUpdateTimeSeconds");
                                
                                if (new_rating && timestamp) {
                                    if ((now - timestamp->valueint) <= days_180) {
                                        recent_180_count++;
                                        if (new_rating->valueint > recent_180_max) {
                                            recent_180_max = new_rating->valueint;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    cJSON_Delete(root);
                }
                free(rating_data);
            }

            char color[16];
            char max_color[16];
            char recent_max_color[16];
            
            if (rating_val < 1200) strcpy(color, "#808080");
            else if (rating_val < 1400) strcpy(color, "#008000");
            else if (rating_val < 1600) strcpy(color, "#00CED1");
            else if (rating_val < 1900) strcpy(color, "#0000FF");
            else if (rating_val < 2200) strcpy(color, "#800080");
            else if (rating_val < 2400) strcpy(color, "#FF8C00");
            else strcpy(color, "#FF0000");

            if (max_val < 1200) strcpy(max_color, "#808080");
            else if (max_val < 1400) strcpy(max_color, "#008000");
            else if (max_val < 1600) strcpy(max_color, "#00CED1");
            else if (max_val < 1900) strcpy(max_color, "#0000FF");
            else if (max_val < 2200) strcpy(max_color, "#800080");
            else if (max_val < 2400) strcpy(max_color, "#FF8C00");
            else strcpy(max_color, "#FF0000");

            if (recent_180_max < 1200) strcpy(recent_max_color, "#808080");
            else if (recent_180_max < 1400) strcpy(recent_max_color, "#008000");
            else if (recent_180_max < 1600) strcpy(recent_max_color, "#00CED1");
            else if (recent_180_max < 1900) strcpy(recent_max_color, "#0000FF");
            else if (recent_180_max < 2200) strcpy(recent_max_color, "#800080");
            else if (recent_180_max < 2400) strcpy(recent_max_color, "#FF8C00");
            else strcpy(recent_max_color, "#FF0000");

            fprintf(out, "        <tr>");
            fprintf(out, "<td><a href=\"%s.html\" style=\"color: %s;\">%s</a></td>", handle, color, handle);
            fprintf(out, "<td>%s</td>", rank_str);
            fprintf(out, "<td style=\"color: %s; font-weight: bold;\">%d</td>", color, rating_val);
            fprintf(out, "<td>%d</td>", contest_count);
            fprintf(out, "<td style=\"color: %s; font-weight: bold;\">%d</td>", max_color, max_val);
            fprintf(out, "<td>%d</td>", recent_180_count);
            fprintf(out, "<td style=\"color: %s; font-weight: bold;\">%d</td>", recent_max_color, recent_180_max);
            fprintf(out, "</tr>\n");
        }
        fclose(f);
    }

    fprintf(out, "    </table>\n");
    
    fprintf(out, "    <div class=\"legend\">\n");
    fprintf(out, "        <h3>等级分颜色说明:</h3>\n");
    fprintf(out, "        <div class=\"legend-item\"><span class=\"legend-color\" style=\"background: #808080;\"></span> Newbie (&lt;1200)</div>\n");
    fprintf(out, "        <div class=\"legend-item\"><span class=\"legend-color\" style=\"background: #008000;\"></span> Pupil (1200-1399)</div>\n");
    fprintf(out, "        <div class=\"legend-item\"><span class=\"legend-color\" style=\"background: #00CED1;\"></span> Specialist (1400-1599)</div>\n");
    fprintf(out, "        <div class=\"legend-item\"><span class=\"legend-color\" style=\"background: #0000FF;\"></span> Expert (1600-1899)</div>\n");
    fprintf(out, "        <div class=\"legend-item\"><span class=\"legend-color\" style=\"background: #800080;\"></span> Candidate Master (1900-2199)</div>\n");
    fprintf(out, "        <div class=\"legend-item\"><span class=\"legend-color\" style=\"background: #FF8C00;\"></span> Master (2200-2399)</div>\n");
    fprintf(out, "        <div class=\"legend-item\"><span class=\"legend-color\" style=\"background: #FF0000;\"></span> Grandmaster (&gt;=2400)</div>\n");
    fprintf(out, "    </div>\n");
    
    fprintf(out, "</div>\n");
    fprintf(out, "</body>\n");
    fprintf(out, "</html>\n");
}

// 辅助函数：JavaScript date 转换
void timestamp_to_date_js(long long timestamp, char *buffer, size_t buffer_size) {
    time_t t = (time_t)timestamp;
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        strftime(buffer, buffer_size, "%Y-%m-%d", tm_info);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <user_handle> [users_file]\n", argv[0]);
        return 1;
    }

    // 初始化 curl
    curl_global_init(CURL_GLOBAL_ALL);

    const char *handle = argv[1];

    // 生成用户个人页面
    char html_file[256];
    snprintf(html_file, sizeof(html_file), "%s.html", handle);
    FILE *out = fopen(html_file, "w");
    if (out) {
        generate_user_summary(handle, out);
        fclose(out);
        printf("Generated %s\n", html_file);
    }

    // 生成多用户列表页面
    if (argc >= 3) {
        // 首先读取所有用户名
        char all_handles[10][128];  // 最多10个用户
        int user_count = 0;
        
        FILE *users_f = fopen(argv[2], "r");
        if (users_f) {
            char user_handle[128];
            while (fgets(user_handle, sizeof(user_handle), users_f) && user_count < 10) {
                user_handle[strcspn(user_handle, "\n")] = 0;
                if (strlen(user_handle) > 0) {
                    strcpy(all_handles[user_count], user_handle);
                    user_count++;
                }
            }
            fclose(users_f);
        }
        
        // 先为其他用户生成页面
        for (int i = 0; i < user_count; i++) {
            char *user_handle = all_handles[i];
            if (strcmp(user_handle, handle) == 0) continue;

            char user_html_file[256];
            snprintf(user_html_file, sizeof(user_html_file), "%s.html", user_handle);

            FILE *user_out = fopen(user_html_file, "w");
            if (user_out) {
                generate_user_summary(user_handle, user_out);
                fclose(user_out);
                printf("Generated %s\n", user_html_file);
            }
        }
        
        // 最后生成index页面
        FILE *index_out = fopen("index.html", "w");
        if (index_out) {
            generate_users_index(argv[2], index_out);
            fclose(index_out);
            printf("Generated index.html\n");
        }
    }

    curl_global_cleanup();
    return 0;
}
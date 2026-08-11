const { chromium } = require('playwright');

(async () => {
    const browser = await chromium.launch({ args: ['--no-sandbox'] });
    const ctx = await browser.newContext({ ignoreHTTPSErrors: true });
    const page = await ctx.newPage();

    const jsErrors = [];
    page.on('pageerror', e => jsErrors.push(e.message));

    const fail = (m) => { console.log('FAIL:', m); browser.close(); process.exit(1); };
    const ok = (m) => console.log('ok  :', m);

    try {
        await page.goto('https://localhost:8443/', { waitUntil: 'networkidle' });

        // 错误密码 → 登录失败提示
        await page.fill('#user', 'admin');
        await page.fill('#pass', 'wrong');
        await page.click('#login-btn');
        await page.waitForFunction(() => document.getElementById('login-error').textContent.length > 0,
            { timeout: 3000 });
        const errText = await page.textContent('#login-error');
        if (!/失败|401|错误/.test(errText)) fail('错误密码应显示失败提示，实际: ' + errText);
        else ok('错误密码 → 登录失败提示: ' + errText.trim());

        // 正确密码登录
        await page.fill('#pass', 'admin');
        await page.click('#login-btn');
        await page.waitForSelector('#conn-status.ok', { timeout: 5000 });
        ok('登录成功，WS 鉴权通过 → ' + (await page.textContent('#conn-status')));

        // 状态渲染
        await page.waitForFunction(() => /PID/.test(document.getElementById('status').textContent),
            { timeout: 5000 });
        ok('状态渲染: ' + (await page.textContent('#status')).replace(/\s+/g, ' ').trim());

        // 指标
        await page.waitForFunction(() => document.querySelectorAll('#metrics .metric').length >= 3,
            { timeout: 5000 });
        const metricN = await page.evaluate(() => document.querySelectorAll('#metrics .metric').length);
        ok('指标卡片: ' + metricN);

        // 日志实时流（订阅后 ~2s 推送）
        await page.waitForFunction(() => document.getElementById('logs').children.length > 0,
            { timeout: 7000 });
        const logN = await page.evaluate(() => document.getElementById('logs').children.length);
        ok('日志实时流: ' + logN + ' 条');

        // 配置修改
        const changed = await page.evaluate(() => {
            const i = document.querySelector('#config input[data-key="log_level"]');
            if (!i) return false;
            i.value = 'debug';
            i.onchange();
            return true;
        });
        ok('配置修改触发 setConfig: ' + changed);

        await page.screenshot({ path: '/tmp/fe-test/dashboard.png' });
        ok('截图: /tmp/fe-test/dashboard.png');

        // restart 流程
        page.on('dialog', async d => { await d.accept(); });
        await page.click('#btn-restart');
        // afterRestart → onclose → "重启中" → pollHealth(~2s) → 回登录页
        await page.waitForFunction(() =>
            /重启中/.test(document.getElementById('conn-status').textContent) ||
            document.getElementById('login-view').style.display !== 'none',
            { timeout: 8000 });
        ok('restart 流程触发（连接关闭/引导重登）');

        if (jsErrors.length) fail('JS 运行时错误: ' + JSON.stringify(jsErrors));
        console.log('\nFRONTEND OK');
    } catch (e) {
        fail(e.message);
    }
    await browser.close();
})();

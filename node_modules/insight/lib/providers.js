'use strict';
const qs = require('querystring');

/**
 * Tracking providers
 *
 * Each provider is a function(id, path) that should return
 * options object for request() call. It will be called bound
 * to Insight instance object.
 */

module.exports = {
	// Google Analytics — https://www.google.com/analytics/
	google(id, payload) {
		const now = Date.now();

		const _qs = {
			// GA Measurement Protocol API version
			v: 1,

			// Hit type
			t: payload.type,

			// Anonymize IP
			aip: 1,

			tid: this.trackingCode,

			// Random UUID
			cid: this.clientId,

			cd1: this.os,

			// GA custom dimension 2 = Node Version, scope = Session
			cd2: this.nodeVersion,

			// GA custom dimension 3 = App Version, scope = Session (temp solution until refactored to work w/ GA app tracking)
			cd3: this.appVersion,

			// Queue time - delta (ms) between now and track time
			qt: now - parseInt(id, 10),

			// Cache busting, need to be last param sent
			z: now
		};

		// Set payload data based on the tracking type
		if (payload.type === 'event') {
			_qs.ec = payload.category;
			_qs.ea = payload.action;

			if (payload.label) {
				_qs.el = payload.label;
			}

			if (payload.value) {
				_qs.ev = payload.value;
			}
		} else {
			_qs.dp = payload.path;
		}

		return {
			url: 'https://ssl.google-analytics.com/collect',
			method: 'POST',
			// GA docs recommends body payload via POST instead of querystring via GET
			body: qs.stringify(_qs)
		};
	},
	// Yandex.Metrica - http://metrica.yandex.com
	yandex(id, payload) {
		const request = require('request');

		const ts = new Date(parseInt(id, 10))
			.toISOString()
			.replace(/[-:T]/g, '')
			.replace(/\..*$/, '');

		const path = payload.path;
		const qs = {
			wmode: 3,
			ut: 'noindex',
			'page-url': `http://${this.packageName}.insight${path}?version=${this.packageVersion}`,
			'browser-info': `i:${ts}:z:0:t:${path}`,
			// Cache busting
			rn: Date.now()
		};

		const url = `https://mc.yandex.ru/watch/${this.trackingCode}`;

		// Set custom cookie using tough-cookie
		const _jar = request.jar();
		const cookieString = `name=yandexuid; value=${this.clientId}; path=/;`;
		const cookie = request.cookie(cookieString);
		_jar.setCookie(cookie, url);

		return {
			url,
			method: 'GET',
			qs,
			jar: _jar
		};
	}
};

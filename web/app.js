/* Giao diện demo độc lập. Các thao tác chưa gọi service C++. */
(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const esc = value => String(value).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const icon = name => `<svg class="icon" aria-hidden="true"><use href="#i-${name}"/></svg>`;
  const number = n => new Intl.NumberFormat('vi-VN').format(n);
  const money = n => `${number(n)} ₫`;
  const date = n => new Intl.DateTimeFormat('vi-VN', {day:'2-digit',month:'2-digit',year:'numeric',timeZone:'Asia/Ho_Chi_Minh'}).format(n);
  const normalize = s => s.normalize('NFD').replace(/[\u0300-\u036f]/g,'').replace(/đ/g,'d').replace(/Đ/g,'D').toLowerCase();
  const statuses = {PENDING:['Chờ xử lý','pending'],PROCESSING:['Đang xử lý','processing'],SHIPPING:['Đang giao','shipping'],COMPLETED:['Hoàn tất','completed'],CANCELLED:['Đã hủy','cancelled']};
  const badge = status => `<span class="badge ${statuses[status][1]}">${statuses[status][0]}</span>`;
  const stockBadge = p => `<span class="badge ${p.stock === 0 ? 'out' : p.stock <= p.minStock ? 'low' : 'normal'}">${p.stock === 0 ? 'Hết hàng' : p.stock <= p.minStock ? 'Sắp hết' : 'Còn hàng'}</span>`;
  const productIcon = p => /Chuot/.test(p.name) ? 'mouse' : /Ban phim/.test(p.name) ? 'keyboard' : /Tai nghe/.test(p.name) ? 'headphones' : /Man hinh|Laptop/.test(p.name) ? 'monitor' : 'chip';
  const key = 'dackdsa-web-demo-v1';
  const products = window.DSA_DATA.products.map(p => ({...p}));
  const orders = window.DSA_DATA.orders.slice();
  const productMap = new Map(products.map(p => [p.id,p]));
  let patch = {orders:[],stocks:{}};
  try {
    const saved = JSON.parse(localStorage.getItem(key));
    // Only restore records produced by this demo, never replace the shared sample.
    if (saved && Array.isArray(saved.orders) && saved.stocks && saved.orders.every(o =>
      /^ORD\d+$/.test(o.orderId) && !orders.some(base => base.orderId === o.orderId) &&
      typeof o.customerName === 'string' && typeof o.customerPhone === 'string' &&
      Number.isSafeInteger(o.createdAt) && Number.isSafeInteger(o.totalAmount) && o.totalAmount >= 0 &&
      [1,2,3].includes(o.priority) && o.status === 'PENDING' && Array.isArray(o.history) &&
      o.history.every(h => h.status === 'PENDING' && Number.isSafeInteger(h.changedAt)) &&
      Array.isArray(o.items) && o.items.length > 0 && o.items.every(i => productMap.has(i.productId) &&
        typeof i.productName === 'string' && Number.isSafeInteger(i.quantity) && i.quantity > 0 && Number.isSafeInteger(i.unitPrice) && i.unitPrice >= 0)) &&
      Object.entries(saved.stocks).every(([id,n]) => productMap.has(id) && Number.isSafeInteger(n) && n >= 0)) {
      patch = saved; orders.push(...saved.orders);
      Object.entries(saved.stocks).forEach(([id,n]) => {productMap.get(id).stock = n;});
    }
  } catch (_) { /* Sample remains available if storage is blocked or invalid. */ }
  const filters = {products:{q:'',stock:'',min:'',max:'',page:1},orders:{q:'',status:'',priority:'',page:1},alerts:{q:'',stock:'',page:1}};
  const labels = {overview:['Tổng quan','Theo dõi hoạt động và những việc cần xử lý.'],products:['Sản phẩm','Tra cứu sản phẩm, giá bán và số lượng trong kho.'],orders:['Đơn hàng','Theo dõi đơn hàng, khách hàng và tiến độ xử lý.'],alerts:['Cảnh báo kho','Kiểm tra những sản phẩm cần bổ sung hàng.']};
  let view = 'overview', rowSequence = 0, toastTimer;
  const lowProducts = () => products.filter(p => p.stock <= p.minStock);
  const sortedOrders = () => orders.slice().sort((a,b) => b.createdAt - a.createdAt || b.orderId.localeCompare(a.orderId));
  function announce(message) { $('announcement').textContent = message; clearTimeout(toastTimer); toastTimer = setTimeout(() => {$('announcement').textContent='';},5000); }
  function counts() {
    $('nav-products').textContent=number(products.length); $('nav-orders').textContent=number(orders.length); $('nav-alerts').textContent=number(lowProducts().length);
  }
  function orderTable(rows) {
    return `<div class="table-wrap"><table><thead><tr><th scope="col">Mã đơn hàng</th><th scope="col">Khách hàng</th><th scope="col">Ngày tạo</th><th scope="col">Trạng thái</th><th scope="col">Tổng tiền</th></tr></thead><tbody>${rows.map(o => `<tr><td><button class="id-button" data-order="${esc(o.orderId)}">${esc(o.orderId)}</button><span class="cell-sub priority-text ${o.priority===1?'high':''}">Ưu tiên ${o.priority}</span></td><td>${esc(o.customerName)}<span class="cell-sub">${esc(o.customerPhone)}</span></td><td>${date(o.createdAt)}</td><td>${badge(o.status)}</td><td>${money(o.totalAmount)}</td></tr>`).join('')}</tbody></table></div>`;
  }
  function productTable(rows) {
    return `<div class="table-wrap"><table><thead><tr><th scope="col">Sản phẩm</th><th scope="col">Đơn giá</th><th scope="col">Tồn kho</th><th scope="col">Ngưỡng cảnh báo</th><th scope="col">Tình trạng</th></tr></thead><tbody>${rows.map(p => `<tr><td><div class="product-cell"><span class="product-icon">${icon(productIcon(p))}</span><div><button class="id-button" data-product="${esc(p.id)}">${esc(p.name)}</button><span class="cell-sub">${esc(p.id)}</span></div></div></td><td>${money(p.price)}</td><td>${number(p.stock)}</td><td>${number(p.minStock)}</td><td>${stockBadge(p)}</td></tr>`).join('')}</tbody></table></div>`;
  }
  function chart() {
    const latest = Math.max(...window.DSA_DATA.orders.map(o=>o.createdAt));
    const dayKey = time => new Intl.DateTimeFormat('en-CA',{timeZone:'Asia/Ho_Chi_Minh',year:'numeric',month:'2-digit',day:'2-digit'}).format(time);
    const days = Array.from({length:14},(_,i)=>latest-(13-i)*86400000);
    const values = days.map(d => orders.filter(o=>o.status==='COMPLETED' && dayKey(o.createdAt)===dayKey(d)).reduce((s,o)=>s+o.totalAmount,0));
    const max = Math.max(...values,1), points = values.map((n,i)=>`${45+i*650/13},${180-n/max*135}`).join(' ');
    return `<div class="chart-summary"><strong>${money(values.reduce((a,b)=>a+b,0))}</strong><span>Giá trị đơn hoàn tất theo ngày tạo</span></div><div class="chart"><svg viewBox="0 0 740 220" role="img" aria-labelledby="chart-title chart-description"><title id="chart-title">Giá trị đơn hoàn tất trong 14 ngày dữ liệu mẫu</title><desc id="chart-description">${days.map((d,i)=>`${date(d)}: ${money(values[i])}`).join('; ')}</desc>${[0,1,2,3].map(i=>`<line x1="45" y1="${45+i*45}" x2="695" y2="${45+i*45}" stroke="#e7eee9"/><text x="0" y="${49+i*45}" fill="#708477" font-size="10">${Math.round(max*(1-i/3)/1e6)}tr</text>`).join('')}<polyline points="${points}" fill="none" stroke="#059669" stroke-width="3" stroke-linejoin="round"/>${[0,4,8,13].map(i=>`<text x="${45+i*650/13}" y="211" fill="#708477" font-size="10" text-anchor="middle">${date(days[i]).slice(0,5)}</text>`).join('')}</svg></div><div class="chart-footer"><span class="chart-legend"><span class="legend-dot"></span>Đơn đã hoàn tất</span><span>${date(days[0]).slice(0,5)} – ${date(days[13])}</span></div>`;
  }
  function overview() {
    const low = lowProducts(), pending = orders.filter(o=>o.status==='PENDING').length, completed = orders.filter(o=>o.status==='COMPLETED').length;
    const stats = [['Tổng sản phẩm',number(products.length),'box',`${number(products.reduce((s,p)=>s+p.stock,0))} đơn vị trong kho`],['Tổng đơn hàng',number(orders.length),'bag',`${number(completed)} đơn đã hoàn tất`],['Đơn chờ xử lý',number(pending),'clock','Sẵn sàng đưa vào xử lý'],['Cần bổ sung hàng',number(low.length),'alert',`${number(low.filter(p=>p.stock===0).length)} sản phẩm đã hết hàng`]];
    $('page-content').innerHTML=`<div class="stats-grid">${stats.map((s,i)=>`<article class="stat-card ${i===3?'warning':''}"><div class="stat-top"><span>${s[0]}</span><span class="stat-icon">${icon(s[2])}</span></div><div class="stat-value">${s[1]}</div><p>${s[3]}</p></article>`).join('')}</div><div class="dashboard-grid"><div class="main-column"><section class="panel"><div class="panel-heading"><div><h2>Giá trị đơn hoàn tất</h2><p>Một góc nhìn về hoạt động bán hàng</p></div><span class="period-tag">${icon('calendar')}14 ngày dữ liệu mẫu</span></div>${chart()}</section><section class="panel"><div class="panel-heading"><div><h2>Đơn hàng gần đây</h2><p>Các đơn mới nhất trong hệ thống</p></div><a class="link-button" href="#orders">Xem tất cả ${icon('arrow')}</a></div>${orderTable(sortedOrders().slice(0,6))}</section></div><div class="side-column"><section class="panel"><div class="panel-heading"><div><h2>Cần bổ sung hàng</h2><p>${number(low.length)} sản phẩm chạm ngưỡng</p></div></div><div class="stock-list">${low.slice(0,4).map(p=>`<div class="stock-item"><span class="product-icon">${icon(productIcon(p))}</span><div class="stock-info"><button class="id-button" data-product="${p.id}">${esc(p.name)}</button><p>${p.id}</p><div class="stock-bottom"><span>Còn ${p.stock} / Ngưỡng ${p.minStock}</span>${stockBadge(p)}</div><div class="stock-track"><span style="width:${Math.min(100,p.stock/Math.max(p.minStock,1)*100)}%"></span></div></div></div>`).join('')}</div><a class="panel-foot-link" href="#alerts">Xem cảnh báo kho ${icon('arrow')}</a></section><section class="panel"><div class="panel-heading"><div><h2>Trạng thái đơn hàng</h2><p>Phân bố ${number(orders.length)} đơn hàng</p></div></div><div class="distribution">${Object.entries(statuses).map(([s,[label,c]])=>{const n=orders.filter(o=>o.status===s).length;return `<div class="distribution-item"><div class="distribution-label"><span>${label}</span><strong>${number(n)}</strong></div><div class="distribution-track"><span class="${c}" style="width:${n/orders.length*100}%"></span></div></div>`;}).join('')}</div></section></div></div>`;
  }
  const options = (items,current) => items.map(([value,label])=>`<option value="${value}" ${value===current?'selected':''}>${label}</option>`).join('');
  function listPage() {
    const f=filters[view], isOrder=view==='orders';
    $('page-content').innerHTML=`${view==='alerts'?`<div class="alert-banner">${icon('alert')}<div><strong>${number(lowProducts().length)} sản phẩm cần chú ý</strong><p>Cảnh báo khi tồn kho nhỏ hơn hoặc bằng ngưỡng, bao gồm hết hàng.</p></div></div>`:''}<section class="panel"><div class="toolbar"><div class="field search-field"><label for="list-search">${isOrder?'Tìm đơn hàng':'Tìm sản phẩm'}</label><div class="input-icon">${icon('search')}<input id="list-search" data-filter="q" value="${esc(f.q)}" placeholder="${isOrder?'Mã đơn hoặc tên khách hàng…':'Mã hoặc tên sản phẩm…'}"></div></div>${isOrder?`<div class="field"><label for="status-filter">Trạng thái</label><select id="status-filter" data-filter="status">${options([['','Tất cả trạng thái'],...Object.entries(statuses).map(([k,v])=>[k,v[0]])],f.status)}</select></div><div class="field"><label for="priority-filter">Ưu tiên</label><select id="priority-filter" data-filter="priority">${options([['','Tất cả mức'],['1','Cao · Mức 1'],['2','Bình thường · Mức 2'],['3','Thấp · Mức 3']],f.priority)}</select></div>`:`<div class="field"><label for="stock-filter">Tồn kho</label><select id="stock-filter" data-filter="stock">${options([['','Tất cả'],['low','Sắp hết / hết hàng'],['out','Hết hàng']],f.stock)}</select></div>${view==='products'?`<div class="field price-field"><label for="price-min">Giá từ (₫)</label><input id="price-min" data-filter="min" type="number" min="0" step="1" value="${esc(f.min)}"></div><div class="field price-field"><label for="price-max">Đến (₫)</label><input id="price-max" data-filter="max" type="number" min="0" step="1" value="${esc(f.max)}"></div>`:''}`}</div><p class="filter-summary" id="filter-summary" role="status" aria-live="polite"></p><div id="list-results"></div></section>`;
    results();
  }
  function results() {
    const f=filters[view], q=normalize(f.q.trim());
    const invalid = view==='products' && ((f.min!=='' && (!Number.isSafeInteger(Number(f.min)) || Number(f.min)<0)) || (f.max!=='' && (!Number.isSafeInteger(Number(f.max)) || Number(f.max)<0)) || (f.min!=='' && f.max!=='' && Number(f.min)>Number(f.max)));
    ['price-min','price-max'].forEach(id=>{if($(id)) $(id).setAttribute('aria-invalid',String(invalid));});
    let rows=view==='orders'?sortedOrders().filter(o=>normalize(`${o.orderId} ${o.customerName} ${o.customerPhone}`).includes(q) && (!f.status || o.status===f.status) && (!f.priority || o.priority===Number(f.priority))):products.filter(p=>normalize(`${p.id} ${p.name}`).includes(q) && (view!=='alerts' || p.stock<=p.minStock) && (!f.stock || (f.stock==='out'?p.stock===0:p.stock<=p.minStock)) && (f.min===undefined || f.min==='' || p.price>=Number(f.min)) && (f.max===undefined || f.max==='' || p.price<=Number(f.max)));
    if(invalid) rows=[];
    const pages=Math.max(1,Math.ceil(rows.length/10)); f.page=Math.min(f.page,pages);
    $('filter-summary').textContent=invalid?'Khoảng giá cần là số nguyên không âm; giá từ không vượt quá giá đến.':`${number(rows.length)} ${view==='orders'?'đơn hàng':'sản phẩm'} phù hợp`;
    $('list-results').innerHTML=rows.length?`${view==='orders'?orderTable(rows.slice((f.page-1)*10,f.page*10)):productTable(rows.slice((f.page-1)*10,f.page*10))}<div class="table-footer"><span>Hiển thị ${(f.page-1)*10+1}–${Math.min(f.page*10,rows.length)} / ${number(rows.length)}</span><div class="pager"><button class="icon-button" data-page="${f.page-1}" aria-label="Trang trước" ${f.page===1?'disabled':''}>${icon('left')}</button><span>Trang ${f.page} / ${pages}</span><button class="icon-button" data-page="${f.page+1}" aria-label="Trang sau" ${f.page===pages?'disabled':''}>${icon('right')}</button></div></div>`:`<div class="empty-state">${icon('search')}<h2>Chưa có kết quả phù hợp</h2><p>Thử một mã khác hoặc thay đổi bộ lọc.</p><button class="button secondary" data-reset>Xóa bộ lọc</button></div>`;
  }
  function render() {
    view=location.hash.slice(1); if(!labels[view]) view='overview';
    $('page-title').textContent=labels[view][0]; $('breadcrumb-current').textContent=labels[view][0]; $('page-description').textContent=labels[view][1];
    document.querySelectorAll('[data-nav]').forEach(a=>{if(a.dataset.nav===view) a.setAttribute('aria-current','page');else a.removeAttribute('aria-current');});
    counts(); view==='overview'?overview():listPage(); $('page-content').setAttribute('aria-busy','false');
  }
  function go(target) {if(location.hash===`#${target}`) render();else location.hash=target;}
  function detailHeader(title,eyebrow) {return `<div class="dialog-heading"><div><p class="eyebrow">${eyebrow}</p><h2 id="detail-title">${esc(title)}</h2></div><button class="icon-button" data-close="detail-dialog" aria-label="Đóng chi tiết" autofocus>${icon('close')}</button></div>`;}
  function showProduct(id) {
    const p=productMap.get(id); if(!p)return;
    $('detail-content').innerHTML=`${detailHeader(p.id,'CHI TIẾT SẢN PHẨM')}<p class="dialog-description">${esc(p.name)}</p><dl class="detail-meta"><div><dt>Đơn giá</dt><dd>${money(p.price)}</dd></div><div><dt>Tồn kho</dt><dd>${number(p.stock)} sản phẩm</dd></div><div><dt>Ngưỡng cảnh báo</dt><dd>${p.minStock}</dd></div><div><dt>Tình trạng</dt><dd>${stockBadge(p)}</dd></div></dl><div class="dialog-footer"><button class="button primary" data-add-product="${p.id}" ${p.stock===0?'disabled':''}>${icon('plus')}Thêm vào đơn</button></div>`;
    $('detail-dialog').showModal();
  }
  function showOrder(id) {
    const o=orders.find(o=>o.orderId===id);if(!o)return;
    $('detail-content').innerHTML=`${detailHeader(o.orderId,'CHI TIẾT ĐƠN HÀNG')}<dl class="detail-meta"><div><dt>Khách hàng</dt><dd>${esc(o.customerName)}</dd></div><div><dt>Số điện thoại</dt><dd>${esc(o.customerPhone)}</dd></div><div><dt>Ngày tạo</dt><dd>${date(o.createdAt)}</dd></div><div><dt>Ưu tiên / Trạng thái</dt><dd>Mức ${o.priority} · ${badge(o.status)}</dd></div></dl><div class="table-wrap detail-table"><table><thead><tr><th scope="col">Sản phẩm</th><th scope="col">SL</th><th scope="col">Đơn giá</th><th scope="col">Thành tiền</th></tr></thead><tbody>${o.items.map(i=>`<tr><td>${esc(i.productName)}<span class="cell-sub">${esc(i.productId)}</span></td><td>${i.quantity}</td><td>${money(i.unitPrice)}</td><td>${money(i.quantity*i.unitPrice)}</td></tr>`).join('')}</tbody></table></div><div class="detail-total"><span>Tổng giá trị</span><strong>${money(o.totalAmount)}</strong></div><h3>Lịch sử trạng thái</h3><ol class="timeline">${o.history.slice().sort((a,b)=>a.changedAt-b.changedAt).map(h=>`<li><span class="timeline-marker">${icon('check')}</span><span>${statuses[h.status][0]}</span><small>${date(h.changedAt)}</small></li>`).join('')}</ol><p class="help-note">Trạng thái hiển thị từ dữ liệu mẫu. Xử lý ưu tiên và chuyển trạng thái đang thực hiện trong chương trình C++.</p>`;
    $('detail-dialog').showModal();
  }
  function addRow(productId='') {
    const seq=++rowSequence;
    $('order-items').insertAdjacentHTML('beforeend',`<div class="order-item-row"><div class="field"><label for="item-${seq}">Mã sản phẩm</label><input id="item-${seq}" class="item-product" list="product-options" value="${esc(productId)}" placeholder="Ví dụ: P001" required autocomplete="off"></div><div class="field"><label for="qty-${seq}">Số lượng</label><input id="qty-${seq}" class="item-quantity" type="number" min="1" step="1" value="1" required></div><button type="button" class="icon-button" data-remove-row aria-label="Xóa dòng sản phẩm">${icon('close')}</button><p class="item-hint"></p></div>`);
    updateTotal();
  }
  function updateTotal() {
    let total=0;
    document.querySelectorAll('.order-item-row').forEach(row=>{const p=productMap.get(row.querySelector('.item-product').value.trim()),qty=Number(row.querySelector('.item-quantity').value); row.querySelector('.item-hint').textContent=p?`${p.name} · ${money(p.price)} · Còn ${p.stock}`:'Nhập mã sản phẩm trong danh mục.';if(p && Number.isSafeInteger(qty) && qty>0)total+=p.price*qty;});
    $('create-total').textContent=Number.isSafeInteger(total)?money(total):'Giá trị vượt giới hạn';
  }
  function openCreate(id='') {$('create-form').reset();$('create-error').hidden=true;$('order-items').innerHTML='';addRow(id);$('create-dialog').showModal();}
  $('product-options').innerHTML=products.map(p=>`<option value="${p.id}">${esc(p.name)}</option>`).join('');
  $('create-order-button').addEventListener('click',()=>openCreate());
  $('add-item-button').addEventListener('click',()=>addRow());
  $('order-items').addEventListener('input',updateTotal);
  $('order-items').addEventListener('click',e=>{const b=e.target.closest('[data-remove-row]');if(b){b.closest('.order-item-row').remove();if(!$('order-items').children.length)addRow();updateTotal();}});
  $('create-form').addEventListener('submit',e=>{
    e.preventDefault(); const fail=message=>{$('create-error').textContent=message;$('create-error').hidden=false;};
    const name=$('customer-name').value.trim(),phone=$('customer-phone').value.trim();if(!name || !phone)return fail('Điền tên khách hàng và số điện thoại.');
    const grouped=new Map();
    for(const row of document.querySelectorAll('.order-item-row')) {
      const id=row.querySelector('.item-product').value.trim(),qty=Number(row.querySelector('.item-quantity').value);
      if(!productMap.has(id))return fail(`Không tìm thấy sản phẩm ${id}. Mã phân biệt hoa/thường.`);
      if(!Number.isSafeInteger(qty) || qty<=0)return fail('Số lượng phải là số nguyên dương.');
      grouped.set(id,(grouped.get(id)||0)+qty);
    }
    let total=0;const items=[];const stocks={...patch.stocks};
    for(const [id,qty] of grouped) {const p=productMap.get(id);if(!Number.isSafeInteger(qty) || qty>p.stock)return fail(`${id} chỉ còn ${p.stock} sản phẩm, không đủ số lượng yêu cầu.`);total+=qty*p.price;stocks[id]=p.stock-qty;items.push({productId:id,productName:p.name,quantity:qty,unitPrice:p.price});}
    if(!Number.isSafeInteger(total))return fail('Tổng tiền vượt giới hạn xử lý của bản demo.');
    const createdAt=Date.now(), orderId=`ORD${String(Math.max(...orders.map(o=>Number(o.orderId.slice(3))))+1).padStart(6,'0')}`;
    const order={orderId,customerName:name,customerPhone:phone,createdAt,totalAmount:total,priority:Number($('order-priority').value),status:'PENDING',items,history:[{status:'PENDING',changedAt:createdAt}]};
    const next={orders:[...patch.orders,order],stocks};
    try {localStorage.setItem(key,JSON.stringify(next));}catch(_){return fail('Trình duyệt không cho lưu dữ liệu hoặc đã đầy. Đơn chưa được tạo.');}
    patch=next;orders.push(order);Object.entries(stocks).forEach(([id,n])=>{productMap.get(id).stock=n;});$('create-dialog').close();go('orders');announce(`Đã tạo đơn thử ${orderId}. Dữ liệu lưu trong trình duyệt.`);
  });
  document.addEventListener('click',e=>{
    const close=e.target.closest('[data-close]');if(close)$(close.dataset.close).close();
    const product=e.target.closest('[data-product]');if(product)showProduct(product.dataset.product);
    const order=e.target.closest('[data-order]');if(order)showOrder(order.dataset.order);
    const add=e.target.closest('[data-add-product]');if(add){$('detail-dialog').close();openCreate(add.dataset.addProduct);}
    const page=e.target.closest('[data-page]');if(page){filters[view].page=Number(page.dataset.page);results();$('list-results').scrollIntoView({block:'nearest'});}
    if(e.target.closest('[data-reset]')){Object.keys(filters[view]).forEach(k=>{filters[view][k]=k==='page'?1:'';});listPage();$('list-search').focus();}
  });
  $('page-content').addEventListener('input',e=>{if(e.target.dataset.filter){filters[view][e.target.dataset.filter]=e.target.value;filters[view].page=1;results();}});
  $('global-search-form').addEventListener('submit',e=>{e.preventDefault();const q=$('global-search').value.trim();const target=/^ORD/i.test(q)?'orders':'products';Object.keys(filters[target]).forEach(k=>{filters[target][k]=k==='page'?1:'';});filters[target].q=q;go(target);});
  $('notifications-button').addEventListener('click',()=>go('alerts'));
  $('help-button').addEventListener('click',()=>$('help-dialog').showModal());
  window.addEventListener('hashchange',()=>{render();$('main').focus({preventScroll:true});});
  render();
})();

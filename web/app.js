/* HTTP API gọi core C++; mở file trực tiếp chỉ xem dữ liệu mẫu. */
(() => {
  'use strict';
  const $ = id => document.getElementById(id);
  const esc = value => String(value).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const icon = name => `<svg class="icon" aria-hidden="true"><use href="#i-${name}"/></svg>`;
  const number = n => new Intl.NumberFormat('vi-VN').format(n);
  const money = n => `${number(n)} ₫`;
  const date = n => new Intl.DateTimeFormat('vi-VN', {day:'2-digit',month:'2-digit',year:'numeric',timeZone:'Asia/Ho_Chi_Minh'}).format(n);
  const dateTime = n => new Intl.DateTimeFormat('vi-VN', {day:'2-digit',month:'2-digit',year:'numeric',hour:'2-digit',minute:'2-digit',second:'2-digit',timeZone:'Asia/Ho_Chi_Minh'}).format(n);
  const normalize = s => s.normalize('NFD').replace(/[\u0300-\u036f]/g,'').replace(/đ/g,'d').replace(/Đ/g,'D').toLowerCase();
  const statuses = {PENDING:['Chờ xử lý','pending'],PROCESSING:['Đang xử lý','processing'],SHIPPING:['Đang giao','shipping'],COMPLETED:['Hoàn tất','completed'],CANCELLED:['Đã hủy','cancelled']};
  const badge = status => `<span class="badge ${statuses[status][1]}">${statuses[status][0]}</span>`;
  const stockBadge = p => `<span class="badge ${p.stock === 0 ? 'out' : p.stock <= p.minStock ? 'low' : 'normal'}">${p.stock === 0 ? 'Hết hàng' : p.stock <= p.minStock ? 'Sắp hết' : 'Còn hàng'}</span>`;
  const productIcon = p => /Chuot/.test(p.name) ? 'mouse' : /Ban phim/.test(p.name) ? 'keyboard' : /Tai nghe/.test(p.name) ? 'headphones' : /Man hinh|Laptop/.test(p.name) ? 'monitor' : 'chip';
  const online = location.protocol === 'http:' || location.protocol === 'https:';
  let products = window.DSA_DATA.products.map(p => ({...p}));
  let orders = window.DSA_DATA.orders.slice();
  let productMap = new Map(products.map(p => [p.id,p]));
  let ready = !online, busy = false, cancelOrder = null, rangeIds = null, rangeSequence = 0;
  let warningProducts = null;
  async function api(path, body) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 15000);
    try {
      const response = await fetch(path, {method:body ? 'POST' : 'GET', cache:'no-store',
        headers:body ? {'Content-Type':'application/json'} : {},
        body:body ? JSON.stringify(body) : undefined, signal:controller.signal});
      const data = await response.json();
      if (!response.ok) throw new Error(data.error || 'Yêu cầu chưa thực hiện được.');
      return data;
    } catch (error) {
      if (error instanceof TypeError || error.name === 'AbortError')
        throw new Error('Mất kết nối server. Nếu vừa gửi thao tác, tải lại dữ liệu để kiểm tra kết quả trước khi gửi lại.');
      throw error;
    } finally {clearTimeout(timer);}
  }
  async function refresh() {
    const data = await api('/api/data');
    products = data.products; orders = data.orders; warningProducts=data.warnings || null; rangeIds=null; ++rangeSequence;
    productMap = new Map(products.map(p => [p.id,p]));
    $('product-options').innerHTML=products.map(p=>`<option value="${esc(p.id)}">${esc(p.name)}</option>`).join('');
    ready = true; $('create-order-button').disabled=false; render();
  }
  async function connect() {
    $('create-order-button').disabled=true;
    try {await refresh();} catch(error) {
      ready=false;
      $('page-content').setAttribute('aria-busy','false');
      $('page-content').innerHTML=`<div class="empty-state" role="alert"><h2>Chưa kết nối được server</h2><p>${esc(error.message)}</p><button class="button secondary" data-reconnect>Thử lại</button></div>`;
    }
  }
  const filters = {products:{q:'',stock:'',category:'',brand:'',min:'',max:'',page:1},orders:{q:'',status:'',priority:'',sort:'newest',page:1},alerts:{q:'',stock:'',category:'',brand:'',page:1}};
  const labels = {overview:['Tổng quan','Theo dõi hoạt động và những việc cần xử lý.'],products:['Sản phẩm','Tra cứu sản phẩm, giá bán và số lượng trong kho.'],orders:['Đơn hàng','Theo dõi đơn hàng, khách hàng và tiến độ xử lý.'],alerts:['Cảnh báo kho','Kiểm tra những sản phẩm cần bổ sung hàng.']};
  let view = 'overview', rowSequence = 0, toastTimer;
  const lowProducts = () => warningProducts || products.filter(p => p.stock <= p.minStock);
  const sortedOrders = () => orders.slice().sort((a,b) => b.createdAt - a.createdAt || b.orderId.localeCompare(a.orderId));
  function announce(message) { $('announcement').textContent = message; clearTimeout(toastTimer); toastTimer = setTimeout(() => {$('announcement').textContent='';},5000); }
  function counts() {
    $('nav-products').textContent=number(products.length); $('nav-orders').textContent=number(orders.length); $('nav-alerts').textContent=number(lowProducts().length);
  }
  function orderTable(rows) {
    return `<div class="table-wrap"><table><thead><tr><th scope="col">Mã đơn hàng</th><th scope="col">Khách hàng</th><th scope="col">Ngày tạo</th><th scope="col">Trạng thái</th><th scope="col">Tổng tiền</th></tr></thead><tbody>${rows.map(o => `<tr><td><button class="id-button" data-order="${esc(o.orderId)}">${esc(o.orderId)}</button><span class="cell-sub priority-text ${o.priority===1?'high':''}">Ưu tiên ${o.priority}</span></td><td>${esc(o.customerName)}<span class="cell-sub">${esc(o.customerPhone)}</span></td><td>${date(o.createdAt)}</td><td>${badge(o.status)}</td><td>${money(o.totalAmount)}</td></tr>`).join('')}</tbody></table></div>`;
  }
  function productTable(rows) {
    return `<div class="table-wrap"><table><thead><tr><th scope="col">Sản phẩm</th><th scope="col">Đơn giá</th><th scope="col">Tồn kho</th><th scope="col">Ngưỡng cảnh báo</th><th scope="col">Tình trạng</th></tr></thead><tbody>${rows.map(p => `<tr><td><div class="product-cell"><span class="product-icon">${icon(productIcon(p))}</span><div><button class="id-button" data-product="${esc(p.id)}">${esc(p.name)}</button><span class="cell-sub">${esc(p.id)}</span><span class="cell-sub">${esc(p.category || 'Chưa phân loại')} · ${esc(p.brand || 'Chưa có thương hiệu')}</span></div></div></td><td>${money(p.price)}</td><td>${number(p.stock)}</td><td>${number(p.minStock)}</td><td>${stockBadge(p)}</td></tr>`).join('')}</tbody></table></div>`;
  }
  function chart() {
    const latest = (orders.length ? Math.max(...orders.map(o=>o.createdAt)) : Date.now());
    const dayKey = time => new Intl.DateTimeFormat('en-CA',{timeZone:'Asia/Ho_Chi_Minh',year:'numeric',month:'2-digit',day:'2-digit'}).format(time);
    const days = Array.from({length:14},(_,i)=>latest-(13-i)*86400000);
    const values = days.map(d => orders.filter(o=>o.status==='COMPLETED' && dayKey(o.createdAt)===dayKey(d)).reduce((s,o)=>s+o.totalAmount,0));
    const max = Math.max(...values,1), points = values.map((n,i)=>`${45+i*650/13},${180-n/max*135}`).join(' ');
    return `<div class="chart-summary"><strong>${money(values.reduce((a,b)=>a+b,0))}</strong><span>Giá trị đơn hoàn tất theo ngày tạo</span></div><div class="chart"><svg viewBox="0 0 740 220" role="img" aria-labelledby="chart-title chart-description"><title id="chart-title">Giá trị đơn hoàn tất trong 14 ngày gần nhất trong dữ liệu</title><desc id="chart-description">${days.map((d,i)=>`${date(d)}: ${money(values[i])}`).join('; ')}</desc>${[0,1,2,3].map(i=>`<line x1="45" y1="${45+i*45}" x2="695" y2="${45+i*45}" stroke="#e7eee9"/><text x="0" y="${49+i*45}" fill="#708477" font-size="10">${Math.round(max*(1-i/3)/1e6)}tr</text>`).join('')}<polyline points="${points}" fill="none" stroke="#059669" stroke-width="3" stroke-linejoin="round"/>${[0,4,8,13].map(i=>`<text x="${45+i*650/13}" y="211" fill="#708477" font-size="10" text-anchor="middle">${date(days[i]).slice(0,5)}</text>`).join('')}</svg></div><div class="chart-footer"><span class="chart-legend"><span class="legend-dot"></span>Đơn đã hoàn tất</span><span>${date(days[0]).slice(0,5)} – ${date(days[13])}</span></div>`;
  }
  function overview() {
    const low = lowProducts(), pending = orders.filter(o=>o.status==='PENDING').length, completed = orders.filter(o=>o.status==='COMPLETED').length;
    const stats = [['Tổng sản phẩm',number(products.length),'box',`${number(products.reduce((s,p)=>s+p.stock,0))} đơn vị trong kho`],['Tổng đơn hàng',number(orders.length),'bag',`${number(completed)} đơn đã hoàn tất`],['Đơn chờ xử lý',number(pending),'clock','Sẵn sàng đưa vào xử lý'],['Cần bổ sung hàng',number(low.length),'alert',`${number(low.filter(p=>p.stock===0).length)} sản phẩm đã hết hàng`]];
    $('page-content').innerHTML=`<div class="stats-grid">${stats.map((s,i)=>`<article class="stat-card ${i===3?'warning':''}"><div class="stat-top"><span>${s[0]}</span><span class="stat-icon">${icon(s[2])}</span></div><div class="stat-value">${s[1]}</div><p>${s[3]}</p></article>`).join('')}</div><div class="dashboard-grid"><div class="main-column"><section class="panel"><div class="panel-heading"><div><h2>Giá trị đơn hoàn tất</h2><p>Một góc nhìn về hoạt động bán hàng</p></div><span class="period-tag">${icon('calendar')}14 ngày gần nhất trong dữ liệu</span></div>${chart()}</section><section class="panel"><div class="panel-heading"><div><h2>Đơn hàng gần đây</h2><p>Các đơn mới nhất trong hệ thống</p></div><a class="link-button" href="#orders">Xem tất cả ${icon('arrow')}</a></div>${orderTable(sortedOrders().slice(0,6))}</section></div><div class="side-column"><section class="panel"><div class="panel-heading"><div><h2>Cần bổ sung hàng</h2><p>${number(low.length)} sản phẩm chạm ngưỡng</p></div></div><div class="stock-list">${low.slice(0,4).map(p=>`<div class="stock-item"><span class="product-icon">${icon(productIcon(p))}</span><div class="stock-info"><button class="id-button" data-product="${p.id}">${esc(p.name)}</button><p>${p.id}</p><div class="stock-bottom"><span>Còn ${p.stock} / Ngưỡng ${p.minStock}</span>${stockBadge(p)}</div><div class="stock-track"><span style="width:${Math.min(100,p.stock/Math.max(p.minStock,1)*100)}%"></span></div></div></div>`).join('')}</div><a class="panel-foot-link" href="#alerts">Xem cảnh báo kho ${icon('arrow')}</a></section><section class="panel"><div class="panel-heading"><div><h2>Trạng thái đơn hàng</h2><p>Phân bố ${number(orders.length)} đơn hàng</p></div></div><div class="distribution">${Object.entries(statuses).map(([s,[label,c]])=>{const n=orders.filter(o=>o.status===s).length;return `<div class="distribution-item"><div class="distribution-label"><span>${label}</span><strong>${number(n)}</strong></div><div class="distribution-track"><span class="${c}" style="width:${n/orders.length*100}%"></span></div></div>`;}).join('')}</div></section></div></div>`;
  }
  const options = (items,current) => items.map(([value,label])=>`<option value="${esc(value)}" ${value===current?'selected':''}>${esc(label)}</option>`).join('');
  function listPage() {
    const f=filters[view], isOrder=view==='orders';
    $('page-content').innerHTML=`${view==='alerts'?`<div class="alert-banner">${icon('alert')}<div><strong>${number(lowProducts().length)} sản phẩm cần chú ý</strong><p>Cảnh báo khi tồn kho nhỏ hơn hoặc bằng ngưỡng, bao gồm hết hàng.</p></div></div>`:''}<div class="list-actions">${isOrder?`<button class="button primary" id="process-next-button" ${!online || busy || !orders.some(o=>o.status==='PENDING')?'disabled':''}>Xử lý đơn ưu tiên tiếp theo</button><p class="muted">Chọn trong toàn bộ đơn chờ: mức 1 → 2 → 3; cùng mức chọn đơn tạo trước.</p>`:`${view==='products'?`<button class="button primary" id="add-product-button" ${!online?'disabled':''}>${icon('plus')}Thêm sản phẩm</button>`:''}`}<button class="button secondary" id="save-csv-button" ${!online?'disabled':''}>Lưu CSV</button><a class="button secondary" href="/api/export" ${!online?'hidden':''}>Tải CSV</a></div><p id="action-error" class="form-error" role="alert" hidden></p><section class="panel"><div class="toolbar"><div class="field search-field"><label for="list-search">${isOrder?'Tìm đơn hàng':'Tìm sản phẩm'}</label><div class="input-icon">${icon('search')}<input id="list-search" data-filter="q" value="${esc(f.q)}" placeholder="${isOrder?'Mã đơn hoặc tên khách hàng…':'Mã hoặc tên sản phẩm…'}"></div></div>${isOrder?`<div class="field"><label for="status-filter">Trạng thái</label><select id="status-filter" data-filter="status">${options([['','Tất cả trạng thái'],...Object.entries(statuses).map(([k,v])=>[k,v[0]])],f.status)}</select></div><div class="field"><label for="priority-filter">Ưu tiên</label><select id="priority-filter" data-filter="priority">${options([['','Tất cả mức'],['1','Cao · Mức 1'],['2','Bình thường · Mức 2'],['3','Thấp · Mức 3']],f.priority)}</select></div><div class="field"><label for="order-sort">Sắp xếp</label><select id="order-sort" data-filter="sort">${options([['newest','Mới nhất trước'],['priority','Ưu tiên cao trước']],f.sort)}</select></div>`:`<div class="field"><label for="category-filter">Loại thiết bị</label><select id="category-filter" data-filter="category">${options([['','Tất cả loại thiết bị'],['__none__','Chưa phân loại'],...classificationValues('category').map(v=>[v,v])],f.category)}</select></div><div class="field"><label for="brand-filter">Thương hiệu</label><select id="brand-filter" data-filter="brand">${options([['','Tất cả thương hiệu'],['__none__','Chưa có thương hiệu'],...classificationValues('brand').map(v=>[v,v])],f.brand)}</select></div><div class="field"><label for="stock-filter">Tồn kho</label><select id="stock-filter" data-filter="stock">${options([['','Tất cả'],['low','Sắp hết / hết hàng'],['out','Hết hàng']],f.stock)}</select></div>${view==='products'?`<div class="field price-field"><label for="price-min">Giá từ (₫)</label><input id="price-min" data-filter="min" type="number" min="0" step="1" value="${esc(f.min)}"></div><div class="field price-field"><label for="price-max">Đến (₫)</label><input id="price-max" data-filter="max" type="number" min="0" step="1" value="${esc(f.max)}"></div><button class="button secondary" id="price-search-button" ${!online?'disabled':''}>Tìm khoảng giá</button>`:''}`}</div><p class="filter-summary" id="filter-summary" role="status" aria-live="polite"></p><div id="list-results"></div></section>`;
    results();
  }
  const classificationValues = key => [...new Set(products.map(p=>p[key]).filter(Boolean))].sort((a,b)=>a.localeCompare(b,'vi'));
  function results() {
    const f=filters[view], q=normalize(f.q.trim());
    const invalid = view==='products' && ((f.min!=='' && (!Number.isSafeInteger(Number(f.min)) || Number(f.min)<0)) || (f.max!=='' && (!Number.isSafeInteger(Number(f.max)) || Number(f.max)<0)) || (f.min!=='' && f.max!=='' && Number(f.min)>Number(f.max)));
    ['price-min','price-max'].forEach(id=>{if($(id)) $(id).setAttribute('aria-invalid',String(invalid));});
    let source=sortedOrders();
    if(view==='orders' && f.sort==='priority')source.sort((a,b)=>a.priority-b.priority || a.createdAt-b.createdAt || a.orderId.length-b.orderId.length || a.orderId.localeCompare(b.orderId));
    let rows=view==='orders'?source.filter(o=>normalize(`${o.orderId} ${o.customerName} ${o.customerPhone}`).includes(q) && (!f.status || o.status===f.status) && (!f.priority || o.priority===Number(f.priority))):(view==='alerts'?lowProducts():products).filter(p=>normalize(`${p.id} ${p.name} ${p.category || ''} ${p.brand || ''}`).includes(q) && (!f.category || (p.category || '__none__')===f.category) && (!f.brand || (p.brand || '__none__')===f.brand) && (!f.stock || (f.stock==='out'?p.stock===0:p.stock<=p.minStock)) && (view!=='products' || !rangeIds || rangeIds.has(p.id)));
    if(invalid) rows=[];
    const pages=Math.max(1,Math.ceil(rows.length/10)); f.page=Math.min(f.page,pages);
    $('filter-summary').textContent=invalid?'Khoảng giá cần là số nguyên không âm; giá từ không vượt quá giá đến.':`${number(rows.length)} ${view==='orders'?'đơn hàng':'sản phẩm'} phù hợp${view==='products' && (f.min!=='' || f.max!=='') && !rangeIds?' · Nhấn Tìm khoảng giá để áp dụng.':''}`;
    $('list-results').innerHTML=rows.length?`${view==='orders'?orderTable(rows.slice((f.page-1)*10,f.page*10)):productTable(rows.slice((f.page-1)*10,f.page*10))}<div class="table-footer"><span>Hiển thị ${(f.page-1)*10+1}–${Math.min(f.page*10,rows.length)} / ${number(rows.length)}</span><div class="pager"><button class="icon-button" data-page="${f.page-1}" aria-label="Trang trước" ${f.page===1?'disabled':''}>${icon('left')}</button><span>Trang ${f.page} / ${pages}</span><button class="icon-button" data-page="${f.page+1}" aria-label="Trang sau" ${f.page===pages?'disabled':''}>${icon('right')}</button></div></div>`:`<div class="empty-state">${icon('search')}<h2>Chưa có kết quả phù hợp</h2><p>Thử một mã khác hoặc thay đổi bộ lọc.</p><button class="button secondary" data-reset>Xóa bộ lọc</button></div>`;
  }
  function render() {
    if (!ready) return;
    view=location.hash.slice(1); if(!labels[view]) view='overview';
    $('page-title').textContent=labels[view][0]; $('breadcrumb-current').textContent=labels[view][0]; $('page-description').textContent=labels[view][1];
    document.querySelectorAll('[data-nav]').forEach(a=>{if(a.dataset.nav===view) a.setAttribute('aria-current','page');else a.removeAttribute('aria-current');});
    counts(); view==='overview'?overview():listPage(); $('page-content').setAttribute('aria-busy','false');
  }
  function go(target) {if(location.hash===`#${target}`) render();else location.hash=target;}
  function detailHeader(title,eyebrow) {return `<div class="dialog-heading"><div><p class="eyebrow">${eyebrow}</p><h2 id="detail-title">${esc(title)}</h2></div><button class="icon-button" data-close="detail-dialog" aria-label="Đóng chi tiết" autofocus>${icon('close')}</button></div>`;}
  function showProduct(id) {
    const p=productMap.get(id); if(!p)return;
    $('detail-content').innerHTML=`${detailHeader(p.id,'CHI TIẾT SẢN PHẨM')}<p class="dialog-description">${esc(p.name)}</p><dl class="detail-meta"><div><dt>Đơn giá</dt><dd>${money(p.price)}</dd></div><div><dt>Tồn kho</dt><dd>${number(p.stock)} sản phẩm</dd></div><div><dt>Loại thiết bị</dt><dd>${esc(p.category || 'Chưa phân loại')}</dd></div><div><dt>Thương hiệu</dt><dd>${esc(p.brand || 'Chưa có thương hiệu')}</dd></div><div><dt>Ngưỡng cảnh báo</dt><dd>${p.minStock}</dd></div><div><dt>Tình trạng</dt><dd>${stockBadge(p)}</dd></div></dl><div class="dialog-footer"><button class="button primary" data-add-product="${esc(p.id)}" ${!online || p.stock===0?'disabled':''}>${icon('plus')}Thêm vào đơn</button></div>`;
    if(online) $('detail-content').querySelector('.dialog-footer').insertAdjacentHTML('afterbegin',`<button class="button secondary" data-edit-product="${esc(p.id)}">Sửa thông tin</button><button class="button secondary" data-stock-product="${esc(p.id)}">Nhập / xuất kho</button><button class="button danger" data-delete-product="${esc(p.id)}">Xóa sản phẩm</button>`);
    if (!$('detail-dialog').open) $('detail-dialog').showModal();
  }
  let productEdit = null;
  function openProductEdit(id, stock, add=false) {
    const p=add?{id:'',name:'',price:0,stock:0,minStock:0,category:'',brand:''}:productMap.get(id); if(!online || !p || busy)return;
    productEdit={id,stock,add,expectedProduct:{...p}};
    const numeric=(id,label,value,min)=>`<div class="field"><label for="${id}">${label}</label><input id="${id}" type="number" min="${min}" max="${Number.MAX_SAFE_INTEGER}" step="1" value="${value}" required aria-describedby="product-edit-error"></div>`;
    $('detail-content').innerHTML=`${detailHeader(add?'Thêm sản phẩm':p.id,stock?'NHẬP / XUẤT KHO':add?'SẢN PHẨM MỚI':'SỬA THÔNG TIN SẢN PHẨM')}<p class="dialog-description">${add?'Nhập thông tin sản phẩm mới.':`${esc(p.name)} · Tồn hiện tại: ${number(p.stock)}.`} ${add?'Mã sản phẩm phải duy nhất, không chứa khoảng trắng.':stock?'Chọn loại thao tác và số lượng cần thay đổi.':'Mã và số lượng tồn giữ nguyên; đơn hàng cũ giữ tên và giá tại thời điểm mua.'}</p><form id="product-edit-form">${add?`<div class="form-grid"><div class="field"><label for="product-new-id">Mã sản phẩm</label><input id="product-new-id" required maxlength="30" aria-describedby="product-edit-error"></div>${numeric('product-new-stock','Tồn kho ban đầu',0,0)}</div>`:''}${stock?`<div class="form-grid"><div class="field"><label for="stock-direction">Thao tác</label><select id="stock-direction"><option value="in">Nhập kho</option><option value="out">Xuất kho</option></select></div>${numeric('stock-quantity','Số lượng',1,1)}</div>`:`<div class="field"><label for="product-edit-name">Tên sản phẩm</label><input id="product-edit-name" value="${esc(p.name)}" maxlength="100" required aria-describedby="product-edit-error"></div><div class="form-grid">${numeric('product-edit-price','Đơn giá (₫)',p.price,0)}${numeric('product-edit-min','Ngưỡng cảnh báo',p.minStock,0)}</div><div class="form-grid"><div class="field"><label for="product-edit-category">Loại thiết bị</label><input id="product-edit-category" list="category-options" value="${esc(p.category || '')}" maxlength="100" placeholder="Ví dụ: Laptop, Chuột…" aria-describedby="product-edit-error"><datalist id="category-options">${classificationValues('category').map(v=>`<option value="${esc(v)}"></option>`).join('')}</datalist></div><div class="field"><label for="product-edit-brand">Thương hiệu</label><input id="product-edit-brand" list="brand-options" value="${esc(p.brand || '')}" maxlength="100" placeholder="Ví dụ: Asus, Logitech…" aria-describedby="product-edit-error"><datalist id="brand-options">${classificationValues('brand').map(v=>`<option value="${esc(v)}"></option>`).join('')}</datalist></div></div>`}<p id="product-edit-error" class="form-error" role="alert" hidden></p><div class="dialog-footer"><button type="button" class="button secondary" data-back-product="${esc(p.id)}">Quay lại</button><button type="submit" class="button primary">${stock?'Cập nhật kho':add?'Thêm sản phẩm':'Lưu thay đổi'}</button></div></form>`;
    if (!$('detail-dialog').open) $('detail-dialog').showModal();
    $(add?'product-new-id':stock?'stock-direction':'product-edit-name').focus();
  }
  document.addEventListener('submit',async e=>{
    if(e.target.id!=='product-edit-form')return;
    e.preventDefault();if(busy || !online || !productEdit)return;
    const {id,stock,add,expectedProduct}=productEdit, form=e.target, error=$('product-edit-error');
    const fail=message=>{error.textContent=message;error.hidden=false;};
    error.hidden=true;
    const inputs=[...form.querySelectorAll('input')];inputs.forEach(i=>i.removeAttribute('aria-invalid'));
    const payload={id,expectedProduct};
    for(const input of form.querySelectorAll('input[type="number"]')) {
      const n=Number(input.value);
      if(!Number.isSafeInteger(n) || n<Number(input.min) || input.value==='') {input.setAttribute('aria-invalid','true');input.focus();return fail('Vui lòng nhập số nguyên trong giới hạn cho phép.');}
    }
    if(stock){payload.quantity=Number($('stock-quantity').value);payload.incoming=$('stock-direction').value==='in';}
    else {
      payload.name=$('product-edit-name').value.trim();
      if(!payload.name){$('product-edit-name').setAttribute('aria-invalid','true');$('product-edit-name').focus();return fail('Tên sản phẩm không được để trống.');}
      payload.price=Number($('product-edit-price').value);payload.minStock=Number($('product-edit-min').value);
      payload.category=$('product-edit-category').value.trim();payload.brand=$('product-edit-brand').value.trim();
      if(add){payload.id=$('product-new-id').value.trim();payload.stock=Number($('product-new-stock').value);if(!payload.id || /\s/.test(payload.id))return fail('Mã sản phẩm không được trống hoặc chứa khoảng trắng.');}
    }
    busy=true;const submit=form.querySelector('[type="submit"]');submit.disabled=true;form.setAttribute('aria-busy','true');
    try {
      const data=await api(add?'/api/products/add':stock?'/api/products/stock':'/api/products/update',payload);
      const index=products.findIndex(p=>p.id===data.product.id);if(index<0)products.push(data.product);else products[index]=data.product;productMap.set(data.product.id,data.product);
      let warning='';try{await refresh();}catch(err){render();warning=` ${err.message}`;}
      if($('detail-dialog').open && $('product-edit-form')===form)showProduct(data.product.id);
      announce(`Đã ${add?'thêm sản phẩm':stock?'cập nhật kho':'sửa thông tin'} ${data.product.id} và lưu dữ liệu.${warning}`);
    } catch(err) {if(form.isConnected)fail(err.message);else announce(err.message);}
    finally {busy=false;submit.disabled=false;form.removeAttribute('aria-busy');}
  });
  function showOrder(id) {
    const o=orders.find(o=>o.orderId===id);if(!o)return;
    $('detail-content').innerHTML=`${detailHeader(o.orderId,'CHI TIẾT ĐƠN HÀNG')}<dl class="detail-meta"><div><dt>Khách hàng</dt><dd>${esc(o.customerName)}</dd></div><div><dt>Số điện thoại</dt><dd>${esc(o.customerPhone)}</dd></div><div><dt>Ngày tạo</dt><dd>${date(o.createdAt)}</dd></div><div><dt>Ưu tiên / Trạng thái</dt><dd>Mức ${o.priority} · ${badge(o.status)}</dd></div></dl><div class="table-wrap detail-table"><table><thead><tr><th scope="col">Sản phẩm</th><th scope="col">SL</th><th scope="col">Đơn giá</th><th scope="col">Thành tiền</th></tr></thead><tbody>${o.items.map(i=>`<tr><td>${esc(i.productName)}<span class="cell-sub">${esc(i.productId)}</span></td><td>${i.quantity}</td><td>${money(i.unitPrice)}</td><td>${money(i.quantity*i.unitPrice)}</td></tr>`).join('')}</tbody></table></div><div class="detail-total"><span>Tổng giá trị</span><strong>${money(o.totalAmount)}</strong></div><h3>Lịch sử trạng thái</h3><ol class="timeline">${o.history.slice().sort((a,b)=>a.changedAt-b.changedAt).map(h=>`<li><span class="timeline-marker">${icon('check')}</span><span>${statuses[h.status][0]}</span><small>${dateTime(h.changedAt)}</small></li>`).join('')}</ol>${statusActions(o)}<p id="status-error" class="form-error" role="alert" hidden></p>`;
    if (!$('detail-dialog').open) $('detail-dialog').showModal();
  }
  function statusActions(o) {
    if (!online) return '<p class="help-note">Mở web qua server để tạo đơn và chuyển trạng thái.</p>';
    const next={PENDING:['PROCESSING','Bắt đầu xử lý'],PROCESSING:['SHIPPING','Chuyển sang đang giao'],SHIPPING:['COMPLETED','Xác nhận hoàn tất']}[o.status];
    return `<div class="dialog-footer">${['PENDING','PROCESSING'].includes(o.status)?`<button class="button danger" data-cancel-order="${esc(o.orderId)}">Hủy đơn</button>`:''}${next?`<button class="button primary" data-status-order="${esc(o.orderId)}" data-status="${next[0]}">${next[1]}</button>`:'<span class="muted">Đơn đã kết thúc, không thể chuyển trạng thái.</span>'}</div>`;
  }
  async function changeStatus(id,target,expectedStatus) {
    if(busy) return; busy=true;
    document.querySelectorAll('[data-status-order],[data-cancel-order],#confirm-cancel').forEach(b=>b.disabled=true);
    const errorElement=target==='CANCELLED'?$('cancel-error'):$('status-error');errorElement.hidden=true;
    try {
      const data=await api(`/api/orders/${encodeURIComponent(id)}/status`,{status:target,expectedStatus});
      const pos=orders.findIndex(o=>o.orderId===id);orders[pos]=data.order;
      $('cancel-dialog').close();
      let refreshError='';
      try {await refresh();} catch(error) {refreshError=error.message;}
      if($('detail-dialog').open)showOrder(id);
      announce(`Đã cập nhật ${id}: ${statuses[target][0]}.${refreshError ? ` ${refreshError}` : ''}`);
    } catch(error) {errorElement.textContent=error.message;errorElement.hidden=false;}
    finally {busy=false;document.querySelectorAll('[data-status-order],[data-cancel-order],#confirm-cancel').forEach(b=>b.disabled=false);}
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
  function openCreate(id='') {if(!online || !ready)return;$('create-form').reset();$('create-error').hidden=true;$('order-items').innerHTML='';addRow(id);$('create-dialog').showModal();}
  $('product-options').innerHTML=products.map(p=>`<option value="${p.id}">${esc(p.name)}</option>`).join('');
  $('create-order-button').addEventListener('click',()=>openCreate());
  $('add-item-button').addEventListener('click',()=>addRow());
  $('order-items').addEventListener('input',updateTotal);
  $('order-items').addEventListener('click',e=>{const b=e.target.closest('[data-remove-row]');if(b){b.closest('.order-item-row').remove();if(!$('order-items').children.length)addRow();updateTotal();}});
  $('create-form').addEventListener('submit',async e=>{
    e.preventDefault(); if(busy || !online)return;
    const fail=message=>{$('create-error').textContent=message;$('create-error').hidden=false;};
    const items=[];
    for(const row of document.querySelectorAll('.order-item-row')) {
      const productId=row.querySelector('.item-product').value.trim(),quantity=Number(row.querySelector('.item-quantity').value);
      if(!Number.isSafeInteger(quantity) || quantity<=0)return fail('Số lượng phải là số nguyên dương.');
      items.push({productId,quantity});
    }
    busy=true;const button=$('create-form').querySelector('[type="submit"]');button.disabled=true;
    try {
      const data=await api('/api/orders',{customerName:$('customer-name').value.trim(),customerPhone:$('customer-phone').value.trim(),priority:Number($('order-priority').value),items});
      $('create-dialog').close();orders.unshift(data.order);
      filters.orders.q='';filters.orders.status='';filters.orders.priority='';filters.orders.page=1;
      try {await refresh();go('orders');announce(`Đã tạo ${data.order.orderId} và lưu CSV.`);}
      catch(error) {go('orders');announce(`Đã lưu đơn ${data.order.orderId}. ${error.message}`);}
    } catch(error) {fail(error.message);}
    finally {busy=false;button.disabled=false;}
  });

  document.addEventListener('click',async e=>{
    const save=e.target.closest('#save-csv-button');
    if(save){if(busy || !online)return;busy=true;save.disabled=true;try{await api('/api/save',{});announce('Đã lưu dữ liệu vào CSV.');}catch(err){$('action-error').textContent=err.message;$('action-error').hidden=false;}finally{busy=false;save.disabled=false;}return;}
    if(e.target.closest('#add-product-button'))openProductEdit('',false,true);
    const price=e.target.closest('#price-search-button');
    if(price){
      if(!online)return;const f=filters.products,min=f.min===''?0:Number(f.min),max=f.max===''?Number.MAX_SAFE_INTEGER:Number(f.max);
      if(!Number.isSafeInteger(min)||!Number.isSafeInteger(max)||min<0||max<min){results();return;}
      const seq=++rangeSequence;price.disabled=true;
      try{const data=await api(`/api/products/range?min=${min}&max=${max}`);if(seq===rangeSequence && view==='products'){rangeIds=new Set(data.products.map(p=>p.id));f.page=1;results();}}
      catch(err){if(seq===rangeSequence){$('action-error').textContent=err.message;$('action-error').hidden=false;}}
      finally{price.disabled=false;}
    }
    const next=e.target.closest('#process-next-button'),remove=e.target.closest('[data-delete-product]');
    if(!next && !remove)return;if(busy || !online)return;
    const p=remove?productMap.get(remove.dataset.deleteProduct):null;
    if(remove && (!p || !confirm(`Xóa sản phẩm ${p.id} — ${p.name}? Sản phẩm đã có trong đơn hàng sẽ không được xóa.`)))return;
    busy=true;const button=next || remove;button.disabled=true;
    try{
      const data=await api(next?'/api/orders/process-next':'/api/products/remove',next?{}:{id:p.id,expectedProduct:{...p}});
      if(remove){products=products.filter(x=>x.id!==p.id);productMap.delete(p.id);$('detail-dialog').close();}
      else{const at=orders.findIndex(o=>o.orderId===data.order.orderId);if(at<0)orders.push(data.order);else orders[at]=data.order;}
      let warning='';try{await refresh();}catch(err){render();warning=` ${err.message}`;}
      if(next){showOrder(data.order.orderId);announce(`Đã chọn ${data.order.orderId}, mức ${data.order.priority}, chuyển sang đang xử lý.${warning}`);}
      else announce(`Đã xóa ${p.id}.${warning}`);
    }catch(err){const target=remove?$('detail-content'):$('page-content');let error=target.querySelector('.operation-error');if(!error){error=document.createElement('p');error.className='form-error operation-error';error.setAttribute('role','alert');target.append(error);}error.textContent=err.message;}
    finally{busy=false;button.disabled=false;const current=$('process-next-button');if(current)current.disabled=!orders.some(o=>o.status==='PENDING');}
  });
  $('confirm-cancel').addEventListener('click',()=>{if(cancelOrder)changeStatus(cancelOrder.id,'CANCELLED',cancelOrder.expected);});
  document.addEventListener('click',e=>{
    const edit=e.target.closest('[data-edit-product]');if(edit)openProductEdit(edit.dataset.editProduct,false);
    const stock=e.target.closest('[data-stock-product]');if(stock)openProductEdit(stock.dataset.stockProduct,true);
    const back=e.target.closest('[data-back-product]');if(back && !busy){if(back.dataset.backProduct)showProduct(back.dataset.backProduct);else $('detail-dialog').close();}
    if(e.target.closest('[data-reconnect]'))connect();
    const status=e.target.closest('[data-status-order]');
    if(status){const o=orders.find(o=>o.orderId===status.dataset.statusOrder);if(o)changeStatus(o.orderId,status.dataset.status,o.status);}
    const cancel=e.target.closest('[data-cancel-order]');
    if(cancel){const o=orders.find(o=>o.orderId===cancel.dataset.cancelOrder);if(o){cancelOrder={id:o.orderId,expected:o.status};$('cancel-description').textContent=`Hủy ${o.orderId}? Các mặt hàng sẽ được hoàn lại vào kho. Đơn đã hủy không thể mở lại.`;$('cancel-error').hidden=true;$('cancel-dialog').showModal();}}
    const close=e.target.closest('[data-close]');if(close)$(close.dataset.close).close();
    const product=e.target.closest('[data-product]');if(product)showProduct(product.dataset.product);
    const order=e.target.closest('[data-order]');if(order)showOrder(order.dataset.order);
    const add=e.target.closest('[data-add-product]');if(add){$('detail-dialog').close();openCreate(add.dataset.addProduct);}
    const page=e.target.closest('[data-page]');if(page){filters[view].page=Number(page.dataset.page);results();$('list-results').scrollIntoView({block:'nearest'});}
    if(e.target.closest('[data-reset]')){rangeIds=null;++rangeSequence;Object.keys(filters[view]).forEach(k=>{filters[view][k]=k==='page'?1:'';});listPage();$('list-search').focus();}
  });
  $('page-content').addEventListener('input',e=>{if(e.target.dataset.filter){if(['min','max'].includes(e.target.dataset.filter)){rangeIds=null;++rangeSequence;}filters[view][e.target.dataset.filter]=e.target.value;filters[view].page=1;results();}});
  $('global-search-form').addEventListener('submit',async e=>{
    e.preventDefault();const id=$('global-search').value.trim();if(!id || !ready)return;
    const isOrder=/^ORD/i.test(id);
    try {
      if(online){const data=await api(`/api/${isOrder?'orders':'products'}/lookup?id=${encodeURIComponent(id)}`);if(isOrder){const at=orders.findIndex(o=>o.orderId===id);if(at<0)orders.push(data.order);else orders[at]=data.order;showOrder(id);}else{productMap.set(id,data.product);showProduct(id);}}
      else{if(isOrder)showOrder(id);else showProduct(id);}
    }catch(error){announce(error.message);}
  });
  $('notifications-button').addEventListener('click',()=>go('alerts'));
  $('help-button').addEventListener('click',()=>$('help-dialog').showModal());
  window.addEventListener('hashchange',()=>{render();$('main').focus({preventScroll:true});});
  if(online){document.querySelector('.demo-tag').innerHTML='<span></span>Core C++';document.querySelector('.page-footer>span:last-child').textContent='Dữ liệu CSV · Đồng bộ qua API';connect();}
  else {$('create-order-button').disabled=true;document.querySelector('.page-footer>span:last-child').textContent='Chỉ xem mẫu · Chạy server để thao tác';render();}
})();

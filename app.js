"use strict";

(() => {
  const STORAGE_KEY = "parksmart.local.v1";
  const LEVELS = ["A", "B", "C"];
  const SLOTS_PER_LEVEL = 5;
  const CAPACITY = LEVELS.length * SLOTS_PER_LEVEL;
  const RATES = { Car: 20, Bike: 10, EV: 25, Truck: 40 };
  const vehicleIcons = {
    Car: '<svg viewBox="0 0 32 18" aria-hidden="true"><path d="m3 11 2-5a2 2 0 0 1 2-1h14a2 2 0 0 1 2 1l3 5v4h-2v-2H5v2H3z"/><path d="M4 11h21M8 14h.01M21 14h.01M11 5l-2 6m7-6 1 6"/></svg>',
    Bike: '<svg viewBox="0 0 32 18" aria-hidden="true"><circle cx="8" cy="12" r="4"/><circle cx="24" cy="12" r="4"/><path d="m8 12 5-7 4 7H8m5-7h4m-1 0 5 7"/></svg>',
    EV: '<svg viewBox="0 0 32 18" aria-hidden="true"><path d="m3 11 2-5a2 2 0 0 1 2-1h14a2 2 0 0 1 2 1l3 5v4h-2v-2H5v2H3z"/><path d="M4 11h21m-9-5-3 4h3l-1 3 4-5h-3l1-2"/></svg>',
    Truck: '<svg viewBox="0 0 32 18" aria-hidden="true"><path d="M3 4h16v10H3zM19 8h6l4 4v2H19z"/><circle cx="8" cy="14" r="2"/><circle cx="24" cy="14" r="2"/></svg>'
  };

  const $ = (selector, root = document) => root.querySelector(selector);
  const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
  const escapeHtml = (value) => String(value).replace(/[&<>"']/g, (character) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  })[character]);
  const formatCurrency = (value) => new Intl.NumberFormat("en-IN", {
    style: "currency", currency: "INR", maximumFractionDigits: 0
  }).format(value);
  const toDate = (value) => new Date(value);
  const formatTime = (value) => toDate(value).toLocaleTimeString([], { hour: "numeric", minute: "2-digit" });
  const formatDateTime = (value) => toDate(value).toLocaleString([], {
    month: "short", day: "numeric", hour: "numeric", minute: "2-digit"
  });
  const isToday = (value) => toDate(value).toDateString() === new Date().toDateString();
  const plateKey = (value) => value.replace(/\s+/g, "").toUpperCase();
  const escapeSelector = (value) => CSS.escape(value);

  function emptyState(title, message) {
    return `<div class="empty-state"><strong>${escapeHtml(title)}</strong><span>${escapeHtml(message)}</span></div>`;
  }

  function loadState() {
    try {
      const stored = localStorage.getItem(STORAGE_KEY);
      if (!stored) return { vehicles: [], waiting: [], entries: [], exits: [] };
      const value = JSON.parse(stored);
      if (!value || !Array.isArray(value.vehicles) || !Array.isArray(value.waiting) ||
          !Array.isArray(value.entries) || !Array.isArray(value.exits)) {
        throw new Error("Saved ParkSmart data has an unexpected format.");
      }
      return value;
    } catch (error) {
      console.error("Unable to load ParkSmart data.", error);
      window.setTimeout(() => toast("Saved data could not be loaded. Check browser storage settings.", "error"), 0);
      return { vehicles: [], waiting: [], entries: [], exits: [] };
    }
  }

  let state = loadState();
  let toastTimer;

  function saveState() {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
      return true;
    } catch (error) {
      console.error("Unable to save ParkSmart data.", error);
      toast("Could not save changes. Browser storage may be full or disabled.", "error");
      return false;
    }
  }

  function toast(message, variant = "success") {
    const region = $("#toast-region");
    const item = document.createElement("div");
    item.className = `toast ${variant}`;
    item.textContent = message;
    region.append(item);
    window.setTimeout(() => item.remove(), 3600);
    if (toastTimer) window.clearTimeout(toastTimer);
    toastTimer = window.setTimeout(() => {
      $$(".toast", region).forEach((toastItem) => toastItem.remove());
    }, 4000);
  }

  function getSlotVehicle(slot) {
    return state.vehicles.find((vehicle) => vehicle.slot === slot);
  }

  function firstFreeSlot() {
    for (const level of LEVELS) {
      for (let number = 1; number <= SLOTS_PER_LEVEL; number += 1) {
        const slot = `${level}${number}`;
        if (!getSlotVehicle(slot)) return slot;
      }
    }
    return null;
  }

  function slotMarkup(slot, compact = false) {
    const vehicle = getSlotVehicle(slot);
    const className = vehicle ? (vehicle.type === "EV" ? "occupied ev-slot" : "occupied") : "free";
    const label = vehicle
      ? `${slot}: ${vehicle.type === "EV" ? "EV charging space" : "Occupied"}, ${vehicle.plate}, ${vehicle.owner}`
      : `${slot}: available`;
    return `<button type="button" class="slot ${className}" data-slot="${slot}" aria-label="${escapeHtml(label)}" ${vehicle ? `data-vehicle-id="${escapeHtml(vehicle.id)}"` : "disabled"}>
      <span class="slot-number">${slot}</span><span class="slot-car">${vehicle ? (vehicleIcons[vehicle.type] || vehicleIcons.Car) : ""}</span>
      ${!compact && vehicle ? `<span class="slot-number">${escapeHtml(vehicle.plate)}</span>` : ""}
    </button>`;
  }

  function renderFloor(target, compact = false) {
    $(target).innerHTML = LEVELS.map((level, index) => {
      const slots = Array.from({ length: SLOTS_PER_LEVEL }, (_, offset) => slotMarkup(`${level}${offset + 1}`, compact)).join("");
      return `<div class="floor-level"><span class="level-label">Level ${index + 1}</span>${slots}</div>`;
    }).join("");
  }

  function renderQueue(target) {
    if (!state.waiting.length) {
      $(target).innerHTML = emptyState("Queue is clear", "New arrivals will appear here if the facility fills up.");
      return;
    }
    $(target).innerHTML = `<div class="queue-content"><div class="queue-pills">${state.waiting.map((vehicle, index) =>
      `<span class="queue-pill"><span class="queue-position">#${index + 1}</span> <strong>${escapeHtml(vehicle.plate)}</strong> ${escapeHtml(vehicle.type)}</span>`
    ).join("")}</div></div>`;
  }

  function renderRecentArrivals() {
    const arrivals = [...state.entries].sort((a, b) => b.at.localeCompare(a.at)).slice(0, 5);
    if (!arrivals.length) {
      $("#recent-arrivals").innerHTML = emptyState("No arrivals yet", "Registered vehicles will show up here.");
      return;
    }
    $("#recent-arrivals").innerHTML = arrivals.map((entry) => `
      <div class="recent-item">
        <span class="vehicle-avatar">${escapeHtml(entry.type === "Truck" ? "TR" : entry.type.slice(0, 2).toUpperCase())}</span>
        <span class="recent-details"><strong>${escapeHtml(entry.plate)}</strong><small>${escapeHtml(entry.owner)} · ${escapeHtml(entry.type)}</small></span>
        <span class="recent-meta"><strong>${escapeHtml(entry.slot)}</strong>${formatTime(entry.at)}</span>
      </div>`).join("");
  }

  function renderVehicleTable() {
    const query = $("#vehicle-search").value.trim().toLowerCase();
    const vehicles = [...state.vehicles].sort((a, b) => a.plate.localeCompare(b.plate));
    const filtered = vehicles.filter((vehicle) =>
      `${vehicle.plate} ${vehicle.owner} ${vehicle.type} ${vehicle.slot}`.toLowerCase().includes(query));
    $("#vehicle-count").textContent = `${state.vehicles.length} ${state.vehicles.length === 1 ? "vehicle" : "vehicles"}`;
    $("#vehicle-table-footer").textContent = `Showing ${filtered.length} of ${state.vehicles.length} parked ${state.vehicles.length === 1 ? "vehicle" : "vehicles"}`;
    if (!filtered.length) {
      $("#vehicle-table-body").innerHTML = `<tr><td colspan="7"><div class="empty-inline">${query ? "No vehicles match your search." : "No vehicles are currently parked."}</div></td></tr>`;
      return;
    }
    $("#vehicle-table-body").innerHTML = filtered.map((vehicle) => `
      <tr>
        <td><span class="vehicle-cell"><span class="vehicle-avatar">${escapeHtml(vehicle.type === "Truck" ? "TR" : vehicle.type.slice(0, 2).toUpperCase())}</span><span><strong>${escapeHtml(vehicle.plate)}</strong><small>Visit ${escapeHtml(vehicle.visit || "1")}</small></span></span></td>
        <td>${escapeHtml(vehicle.owner)}</td><td><span class="type-badge ${vehicle.type === "EV" ? "ev" : vehicle.type === "Truck" ? "truck" : ""}">${escapeHtml(vehicle.type)}</span></td>
        <td><span class="space-label"><i></i>${escapeHtml(vehicle.slot)}</span></td><td>${formatDateTime(vehicle.entryAt)}</td>
        <td><span class="status-badge">Parked</span></td><td><button class="row-action" type="button" data-checkout="${escapeHtml(vehicle.id)}">Check out</button></td>
      </tr>`).join("");
  }

  function eventStream() {
    const entries = state.entries.map((entry) => ({
      kind: entry.queued ? "queue" : "entry",
      at: entry.at,
      title: entry.queued ? `${entry.plate} joined the waiting queue` : `${entry.plate} checked in`,
      detail: entry.queued ? `${entry.owner} · ${entry.type} · Waiting list` : `${entry.owner} · ${entry.type} · Space ${entry.slot}`
    }));
    const exits = state.exits.map((exit) => ({
      kind: "exit", at: exit.at,
      title: `${exit.plate} checked out`,
      detail: `${exit.owner} · ${exit.duration} billed ${exit.duration === 1 ? "hour" : "hours"} · ${formatCurrency(exit.fee)}`
    }));
    return [...entries, ...exits].sort((a, b) => b.at.localeCompare(a.at));
  }

  function renderActivity() {
    const events = eventStream();
    $("#history-entry-count").textContent = state.entries.length;
    $("#history-exit-count").textContent = state.exits.length;
    $("#history-waiting-count").textContent = state.waiting.length;
    if (!events.length) {
      $("#activity-list").innerHTML = emptyState("No activity recorded", "Vehicle arrivals and check-outs will appear here.");
      return;
    }
    $("#activity-list").innerHTML = events.map((event) => `
      <div class="activity-row"><span class="activity-symbol ${event.kind}">${event.kind === "exit" ? "↗" : event.kind === "queue" ? "◷" : "↘"}</span>
      <span class="activity-copy"><strong>${escapeHtml(event.title)}</strong><small>${escapeHtml(event.detail)}</small></span>
      <span class="activity-time"><strong>${formatTime(event.at)}</strong>${toDate(event.at).toLocaleDateString([], { month: "short", day: "numeric" })}</span></div>`).join("");
  }

  function renderReports() {
    const occupied = state.vehicles.length;
    const percent = Math.round(occupied / CAPACITY * 100);
    const arrivalsToday = state.entries.filter((entry) => isToday(entry.at)).length;
    const revenue = state.exits.reduce((sum, exit) => sum + exit.fee, 0);
    $("#report-occupancy").textContent = `${percent}%`;
    $("#report-meter").style.width = `${percent}%`;
    $("#report-arrivals").textContent = arrivalsToday;
    $("#report-revenue").textContent = formatCurrency(revenue);
    $("#report-date").textContent = new Date().toLocaleDateString([], { weekday: "long", month: "long", day: "numeric", year: "numeric" });
    const counts = Object.fromEntries(Object.keys(RATES).map((type) => [type, state.vehicles.filter((vehicle) => vehicle.type === type).length]));
    const maxCount = Math.max(1, ...Object.values(counts));
    $("#vehicle-mix").innerHTML = Object.entries(counts).map(([type, count]) => `
      <div class="mix-row"><span>${type === "EV" ? "Electric vehicle" : type}</span><span class="mix-track"><i style="width:${count / maxCount * 100}%"></i></span><strong>${count}</strong></div>`).join("");
    const exits = [...state.exits].sort((a, b) => b.at.localeCompare(a.at)).slice(0, 6);
    $("#report-checkouts").innerHTML = exits.length ? exits.map((exit) => `
      <div class="activity-row"><span class="activity-symbol exit">↗</span><span class="activity-copy"><strong>${escapeHtml(exit.plate)} · ${escapeHtml(exit.owner)}</strong><small>${escapeHtml(exit.duration)} billed ${exit.duration === 1 ? "hour" : "hours"} · ${formatDateTime(exit.at)}</small></span><span class="activity-time"><strong>${formatCurrency(exit.fee)}</strong></span></div>`).join("") : '<div class="empty-inline">Completed check-outs and fees will appear here.</div>';
  }

  function render() {
    const occupied = state.vehicles.length;
    const available = CAPACITY - occupied;
    const percent = Math.round(occupied / CAPACITY * 100);
    const arrivalsToday = state.entries.filter((entry) => isToday(entry.at)).length;
    const exitsToday = state.exits.filter((exit) => isToday(exit.at)).length;
    $("#metric-occupied").textContent = occupied;
    $("#metric-capacity").textContent = `of ${CAPACITY} spaces`;
    $("#metric-available").textContent = available;
    $("#metric-waiting").textContent = state.waiting.length;
    $("#metric-arrivals").textContent = arrivalsToday;
    $("#metric-exits").textContent = `${exitsToday} ${exitsToday === 1 ? "departure" : "departures"} today`;
    $("#occupancy-percent").textContent = `${percent}% full`;
    $("#occupancy-meter").style.width = `${percent}%`;
    $("#vehicle-nav-count").textContent = occupied;
    $("#queue-count").textContent = `${state.waiting.length} waiting`;
    $("#vehicle-queue-count").textContent = `${state.waiting.length} waiting`;
    $("#activity-queue-count").textContent = `${state.waiting.length} waiting`;
    $("#floor-summary").innerHTML = `<div class="summary-chip"><span>Total capacity</span><strong>${CAPACITY} spaces</strong></div><div class="summary-chip"><span>Currently occupied</span><strong>${occupied} spaces</strong></div><div class="summary-chip"><span>Available now</span><strong>${available} spaces</strong></div>`;
    $("#floor-vehicle-list").innerHTML = occupied
      ? `<div class="table-scroll"><table><thead><tr><th>VEHICLE</th><th>OWNER</th><th>TYPE</th><th>SPACE</th><th>CHECKED IN</th><th></th></tr></thead><tbody>${state.vehicles.map((vehicle) => `<tr><td class="plate">${escapeHtml(vehicle.plate)}</td><td>${escapeHtml(vehicle.owner)}</td><td>${escapeHtml(vehicle.type)}</td><td>${escapeHtml(vehicle.slot)}</td><td>${formatDateTime(vehicle.entryAt)}</td><td><button class="row-action" type="button" data-checkout="${escapeHtml(vehicle.id)}">Check out</button></td></tr>`).join("")}</tbody></table></div>`
      : '<div class="empty-inline">No vehicles are parked right now.</div>';
    renderFloor("#dashboard-floor", true);
    renderFloor("#full-floor");
    renderRecentArrivals();
    renderQueue("#dashboard-queue");
    renderQueue("#vehicle-queue");
    renderQueue("#activity-queue");
    renderVehicleTable();
    renderActivity();
    renderReports();
  }

  function navigate(page) {
    const pageTitles = { dashboard: "Dashboard", parking: "Parking floor", vehicles: "Vehicle directory", activity: "Activity", reports: "Reports" };
    if (!pageTitles[page]) return;
    $$(".page").forEach((section) => section.classList.toggle("active", section.id === `page-${page}`));
    $$(".nav-item").forEach((button) => button.classList.toggle("active", button.dataset.page === page));
    $("#breadcrumb-current").textContent = pageTitles[page];
    document.title = `ParkSmart | ${pageTitles[page]}`;
    window.scrollTo({ top: 0, behavior: "smooth" });
  }

  function registerVehicle(formData) {
    const plate = String(formData.get("plate") || "").trim().toUpperCase();
    const owner = String(formData.get("owner") || "").trim();
    const type = String(formData.get("type") || "");
    if (!plate || !owner || !Object.hasOwn(RATES, type)) {
      toast("Enter a registration number, owner, and valid vehicle type.", "error");
      return;
    }
    const key = plateKey(plate);
    if ([...state.vehicles, ...state.waiting].some((vehicle) => plateKey(vehicle.plate) === key)) {
      toast("That registration is already parked or waiting.", "error");
      return;
    }
    const timestamp = new Date().toISOString();
    const vehicle = { id: crypto.randomUUID(), plate, owner, type, entryAt: timestamp, visit: "1" };
    const slot = firstFreeSlot();
    if (slot) {
      vehicle.slot = slot;
      state.vehicles.push(vehicle);
      state.entries.push({ ...vehicle, at: timestamp, queued: false });
    } else {
      state.waiting.push(vehicle);
      state.entries.push({ ...vehicle, at: timestamp, queued: true });
    }
    if (!saveState()) return;
    $("#park-dialog").close();
    $("#park-form").reset();
    render();
    if (slot) {
      toast(`${plate} parked successfully in space ${slot}.`);
    } else {
      toast(`${plate} added to the waiting queue.`, "error");
    }
  }

  function checkoutVehicle(id) {
    const vehicleIndex = state.vehicles.findIndex((item) => item.id === id);
    if (vehicleIndex < 0) {
      toast("This vehicle is no longer parked.", "error");
      return;
    }
    const vehicle = state.vehicles[vehicleIndex];
    const now = new Date();
    const elapsed = Math.max(0, now.getTime() - toDate(vehicle.entryAt).getTime());
    const duration = Math.max(1, Math.ceil(elapsed / 3600000));
    const fee = duration * RATES[vehicle.type];
    state.vehicles.splice(vehicleIndex, 1);
    state.exits.push({ ...vehicle, at: now.toISOString(), duration, fee });
    let promoted = null;
    if (state.waiting.length) {
      promoted = state.waiting.shift();
      promoted.slot = vehicle.slot;
      promoted.entryAt = now.toISOString();
      state.vehicles.push(promoted);
      state.entries.push({ ...promoted, at: promoted.entryAt, queued: false });
    }
    if (!saveState()) return;
    render();
    toast(`${vehicle.plate} checked out. Fee: ${formatCurrency(fee)}.${promoted ? ` ${promoted.plate} moved into ${promoted.slot}.` : ""}`);
  }

  function showVehicleDetails(id) {
    const vehicle = state.vehicles.find((item) => item.id === id);
    if (!vehicle) return;
    $("#details-content").innerHTML = `
      <div class="details-titlebar"><div><div class="eyebrow">PARKED VEHICLE</div><h2 id="details-title">${escapeHtml(vehicle.plate)}</h2><p>${escapeHtml(vehicle.owner)}</p></div><button class="icon-button close-button" type="button" data-close-details aria-label="Close details">×</button></div>
      <div class="detail-grid"><div class="detail-item"><small>Vehicle type</small><strong>${escapeHtml(vehicle.type)}</strong></div><div class="detail-item"><small>Parking space</small><strong>${escapeHtml(vehicle.slot)}</strong></div><div class="detail-item"><small>Checked in</small><strong>${formatDateTime(vehicle.entryAt)}</strong></div><div class="detail-item"><small>Hourly rate</small><strong>${formatCurrency(RATES[vehicle.type])} / hour</strong></div></div>
      <div class="details-actions"><button class="button button-quiet" type="button" data-close-details>Close</button><button class="button button-primary" type="button" data-detail-checkout="${escapeHtml(vehicle.id)}">Check out vehicle</button></div>`;
    $("#details-dialog").showModal();
  }

  function resetLocalData() {
    if (!window.confirm("Clear all ParkSmart vehicles, queue entries, and history saved in this browser? This cannot be undone.")) return;
    const previous = state;
    state = { vehicles: [], waiting: [], entries: [], exits: [] };
    if (!saveState()) {
      state = previous;
      return;
    }
    render();
    toast("Local demo data cleared.");
  }

  document.addEventListener("click", (event) => {
    const target = event.target instanceof Element ? event.target : null;
    if (!target) return;
    const pageButton = target.closest("[data-page]");
    if (pageButton) navigate(pageButton.dataset.page);
    const pageLink = target.closest("[data-page-link]");
    if (pageLink) {
      event.preventDefault();
      navigate(pageLink.dataset.pageLink);
    }
    const navigateButton = target.closest("[data-navigate]");
    if (navigateButton) navigate(navigateButton.dataset.navigate);
    if (target.closest("[data-open-park]")) {
      $("#park-dialog").showModal();
      $("#plate-input").focus();
    }
    if (target.closest("[data-close-dialog]")) $("#park-dialog").close();
    if (target.closest("[data-close-details]")) $("#details-dialog").close();
    const checkoutButton = target.closest("[data-checkout]");
    if (checkoutButton) checkoutVehicle(checkoutButton.dataset.checkout);
    const detailCheckout = target.closest("[data-detail-checkout]");
    if (detailCheckout) {
      $("#details-dialog").close();
      checkoutVehicle(detailCheckout.dataset.detailCheckout);
    }
    const slot = target.closest("[data-vehicle-id]");
    if (slot) showVehicleDetails(slot.dataset.vehicleId);
    if (target.closest("#reset-button")) resetLocalData();
  });

  $("#park-form").addEventListener("submit", (event) => {
    event.preventDefault();
    registerVehicle(new FormData(event.currentTarget));
  });
  $("#vehicle-search").addEventListener("input", renderVehicleTable);
  $("#park-dialog").addEventListener("click", (event) => {
    if (event.target === event.currentTarget) event.currentTarget.close();
  });
  $("#details-dialog").addEventListener("click", (event) => {
    if (event.target === event.currentTarget) event.currentTarget.close();
  });
  $("#today-label").textContent = new Date().toLocaleDateString([], { weekday: "short", month: "short", day: "numeric", year: "numeric" });
  render();
})();

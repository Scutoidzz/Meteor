(function () {
  const nextButtons = document.querySelectorAll('[data-next-page]');
  nextButtons.forEach((btn) => {
    btn.addEventListener('click', () => {
      const target = btn.getAttribute('data-next-page');
      if (target) {
        window.location.href = target;
      }
    });
  });
})();

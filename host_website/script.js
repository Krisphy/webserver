document.addEventListener('DOMContentLoaded', function() {
    console.log('Website loaded successfully!');
    
    // Add a click event listener to all links
    const links = document.querySelectorAll('a');
    links.forEach(link => {
        link.addEventListener('click', function(event) {
            console.log('Clicked link: ' + this.href);
        });
    });
});

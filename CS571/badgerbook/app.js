fetch('https://cs571.org/api/s24/hw2/students', {
	headers: {
		'X-CS571-ID': CS571.getBadgerId()
	}
})
	.then(res => {
		console.log(res.status, res.statusText);
		if (res.status === 200) {
			return res.json();
		} else {
			throw new Error();
		}
	})
	.then(data => {
		console.log(data);
		document.getElementById('num-results').innerText = data.length;
		buildStudents(data);
	})
	.catch(err => {
		console.error('Error fetching or processing data:', err);
	});

function buildStudents(studs) {
	// TODO This function is just a suggestion! I would suggest calling it after
	//      fetching the data or performing a search. It should populate the
	//      index.html with student data by using createElement and appendChild.
	const container = document.getElementById('students');
	container.innerHTML = '';
	container.className = 'row';

	studs.forEach(student => {
		const studentDiv = document.createElement('div');
		studentDiv.className = 'col-12 col-sm-6 col-md-4 col-lg-3 col-xl-3';

		const studentInfo = document.createElement('div');
		const nameElement = document.createElement('p');
		const majorElement = document.createElement('p');
		const interestsElement = document.createElement('ul');
		const creditsElement = document.createElement('p');

		//setting text content for each element
		nameElement.textContent = `${student.name.first || 'Unknown'} ${student.name.last || 'Unknown'}`;
		majorElement.textContent = `Major: ${student.major || 'Undefined'}`;

		const firstLineText = `${student.name.first || 'Unknown'} ${student.name.last || 'Unknown'} is taking ${student.numCredits || 'Unknown'} credits and is ${student.fromWisconsin ? 'from Wisconsin' : 'not from Wisconsin'}.`;
		const firstLineElement = document.createElement('p');
		firstLineElement.textContent = firstLineText;

		const secondLineText = `They have ${student.interests.length} interests including:`;
		const secondLineElement = document.createElement('p');
		secondLineElement.textContent = secondLineText;

		//list items for interests
		student.interests.forEach(interest => {
			const interestItem = document.createElement('li');
			const interestItemAnchor = document.createElement('a');
			interestItemAnchor.textContent = interest;
			interestItem.appendChild(interestItemAnchor);
			interestsElement.appendChild(interestItem);

			//click event listener to interest item
			interestItemAnchor.addEventListener("click", (e) => {
				const selectedInterest = e.target.innerText;
				updateSearch(selectedInterest);
			});
		});

		//to make names bold
		nameElement.innerHTML = `<strong style="font-size: 25; font-weight: bold;">${student.name.first || 'Unknown'} 
						${student.name.last || 'Unknown'}</strong>`;
		majorElement.innerHTML = `<strong style="font-size: 18; font-weight: bold;">${student.major || 'Unknown'}</strong>`;
		creditsElement.innerHTML = `<strong style="font-size: 15;">${student.numCredits}</strong>`;

		//append elements to student info container
		studentInfo.appendChild(nameElement);
		studentInfo.appendChild(majorElement);
		studentInfo.appendChild(firstLineElement);
		studentInfo.appendChild(secondLineElement);
		studentInfo.appendChild(interestsElement);


		studentDiv.appendChild(studentInfo);
		container.appendChild(studentDiv);
	});
}

function updateSearch(selectedInterest) {
	//get search input fields
	const nameInput = document.getElementById('search-name').value.trim().toLowerCase();
	const majorInput = document.getElementById('search-major').value.trim().toLowerCase();
	document.getElementById('search-interest').value = selectedInterest;
	handleSearch();
}

function handleSearch(e) {
	e?.preventDefault(); // You can ignore this; prevents the default form submission!
	// TODO Implement the search   
	fetch("https://cs571.org/api/s24/hw2/students", {
		headers: {
			'X-CS571-ID': CS571.getBadgerId()
		}
	})
		.then((res) => {
			return res.json();

		})
		.then((data) => {
			const name = document.getElementById('search-name').value.trim();
			const major = document.getElementById('search-major').value.trim();
			const interest = document.getElementById('search-interest').value.trim();
			const filterdata = data.filter(studs => {
				const firstLast = `${studs.name.first} ${studs.name.last}`
				const namesearch = name ? firstLast.toLowerCase().includes(name.toLowerCase()) : true;
				const majorsearch = major ? studs.major.toLowerCase().includes(major.toLowerCase()) : true;
				const interestsearch = interest ? studs.interests.some(eachInterest => eachInterest.toLowerCase().includes(interest.toLowerCase())) : true;

				return namesearch && majorsearch && interestsearch;
			})
			buildStudents(filterdata)
			document.getElementById('num-results').innerText = filterdata.length
		})
		.catch(error => {
			console.error('error retrieving data', error);
		});
}
document.getElementById("search-btn").addEventListener("click", handleSearch);
